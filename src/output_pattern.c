/**
 * @file output_pattern.c
 * @brief Pattern sur une sortie logique.
 *
 * Noyau commun à output_pattern et à output_pwm_pattern : le déroulement de la
 * séquence (étapes, cycles, pause, arrêt, détection des échecs) est identique
 * pour les deux modules. Seul l'adaptateur installé par init() change, et il
 * n'est appelé qu'au changement de consigne, jamais à chaque tick.
 */

#include "libpic/output_pattern.h"

static OutputPatternEvent_t snapshot(OutputPattern_t *pattern);

/* Adaptateur logique : toute consigne non nulle active la sortie. */
static void apply_logic(void *output, uint8_t value)
{
	output_set((output_t *)output, value != 0u);
}

/* Écrit la consigne via l'adaptateur ; OUTPUT_KEEP ne touche pas la sortie. */
static void run_apply(OutputPattern_t *pattern, uint8_t value)
{
	if (pattern->output == NULL || pattern->apply == NULL || value == OUTPUT_KEEP) return;

	pattern->apply(pattern->output, value);
}

bool output_pattern_init(OutputPattern_t *pattern, output_t *output)
{
	if (pattern == NULL || output == NULL) return false;

	pattern->output = output;
	pattern->apply = apply_logic;
	output_pattern_reset(pattern);

	return true;
}

OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms)
{
	OutputPatternEvent_t event = snapshot(pattern);

	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) return event;

	if (pattern->execution_state == PATTERN_EXEC_ABORDED ||
		pattern->execution_state == PATTERN_EXEC_COMPLETED ||
		pattern->execution_state == PATTERN_EXEC_STOPPED) return event;

	OutputPatternStepState_t *current_state = &pattern->states[pattern->current_step_id];
	const OutputPatternStep_t *current_step = &pattern->sequence->steps[pattern->current_step_id];

	time_ms_t elapsed_ms = (time_ms_t)(now_ms - current_state->step_start_ms);

	// Étape en cours non terminée : la sortie conserve la consigne appliquée à son démarrage
	if (elapsed_ms < current_step->duration_ms) return event;

	// Écart entre la fin théorique de l'étape et l'instant réel de validation
	time_ms_t overrun_ms = (time_ms_t)(elapsed_ms - current_step->duration_ms);

	// Durée de la phase qui aurait dû démarrer à temps (0 si le pattern se termine)
	time_ms_t next_duration_ms = 0;
	if (pattern->current_step_id + 1u < pattern->sequence->length) {
		next_duration_ms = pattern->sequence->steps[pattern->current_step_id + 1u].duration_ms;
	} else if (pattern->cycle_target == PATTERN_UNLIMITED_CYCLES ||
	           pattern->cycles_done + 1u < pattern->cycle_target) {
		next_duration_ms = pattern->sequence->steps[0].duration_ms;
	}

	// Une phase entière (ou plus) a été avalée depuis le dernier appel.
	// Le contrat temporel (appeler update au moins une fois par durée d'étape)
	// vaut aussi en pause : seule la sortie y est figée.
	bool step_late = (next_duration_ms > 0u) && (overrun_ms >= next_duration_ms);

	if (step_late) {
		current_state->state = OUTPUT_STEP_FAILED;
		event.new_failed = true;
	} else if (pattern->execution_state == PATTERN_EXEC_RUNNING) {
		current_state->state = OUTPUT_STEP_COMPLETED;
	} else {
		current_state->state = OUTPUT_STEP_SKIPPED;
		event.new_skipped = true;
	}

	// Le dépassement est une erreur pour l'utilisateur : on interrompt le pattern
	if (step_late && pattern->abort_on_fail) {
		pattern->execution_state = PATTERN_EXEC_ABORDED;
		event.pattern_state = PATTERN_EXEC_ABORDED;

		run_apply(pattern, pattern->state_on_fail);
		return event;
	}

	// Dans les deux cas (running et paused), on pointe l'étape suivante
	if (pattern->current_step_id + 1u < pattern->sequence->length) {
		pattern->current_step_id++;
	} else {
		pattern->cycles_done++;

		// Dernier cycle réalisé : le pattern est complet, la sortie conserve la
		// consigne de la dernière étape
		if (pattern->cycle_target != PATTERN_UNLIMITED_CYCLES && pattern->cycles_done >= pattern->cycle_target) {
			pattern->execution_state = PATTERN_EXEC_COMPLETED;

			event.pattern_state = pattern->execution_state;
			event.current_step = pattern->current_step_id;
			event.cycles_done = pattern->cycles_done;

			return event;
		}

		// Nouveau cycle : les états des étapes repartent de zéro
		pattern->current_step_id = 0;

		for (uint8_t i = 0; i < pattern->sequence->length; ++i)
		{
			pattern->states[i] = (OutputPatternStepState_t){0};
		}
	}

	current_state = &pattern->states[pattern->current_step_id];
	current_step = &pattern->sequence->steps[pattern->current_step_id];

	current_state->state = OUTPUT_STEP_RUNNING;
	current_state->step_start_ms = now_ms;

	event.pattern_state = pattern->execution_state;
	event.current_step = pattern->current_step_id;
	event.cycles_done = pattern->cycles_done;

	// La consigne de la nouvelle étape n'est appliquée qu'en marche : en pause,
	// la sortie reste figée sur la consigne imposée par output_pattern_pause()
	if (pattern->execution_state == PATTERN_EXEC_RUNNING) {
		run_apply(pattern, current_step->value);
	}

	return event;
}

bool output_pattern_run(OutputPattern_t *pattern,
                        const OutputPatternSequence_t *sequence,
                        OutputPatternStepState_t *states,
                        uint8_t states_count,
                        uint8_t cycle_target,
                        bool abort_on_fail,
                        uint8_t state_on_fail,
                        time_ms_t now_ms)
{
	if (pattern == NULL ||
		sequence == NULL ||
		sequence->steps == NULL ||
		states == NULL ||
		sequence->length == 0 ||
		states_count != sequence->length)
	{
		return false;
	}

	pattern->sequence = sequence;
	pattern->states = states;
	pattern->cycle_target = cycle_target;
	pattern->abort_on_fail = abort_on_fail;
	pattern->state_on_fail = state_on_fail;

	output_pattern_reset(pattern);

	OutputPatternStepState_t *current_state = &pattern->states[pattern->current_step_id];
	const OutputPatternStep_t *current_step = &pattern->sequence->steps[pattern->current_step_id];

	pattern->execution_state = PATTERN_EXEC_RUNNING;
	current_state->state = OUTPUT_STEP_RUNNING;
	current_state->step_start_ms = now_ms;

	run_apply(pattern, current_step->value);

	return true;
}

void output_pattern_reset(OutputPattern_t *pattern)
{
	if (pattern == NULL) return;

	pattern->execution_state = PATTERN_EXEC_STOPPED;
	pattern->cycles_done = 0;
	pattern->current_step_id = 0;
	pattern->flag_any_failed = false;
	pattern->flag_any_skipped = false;

	if (pattern->sequence == NULL || pattern->states == NULL) return;

	for (uint8_t i = 0; i < pattern->sequence->length; ++i)
	{
		pattern->states[i] = (OutputPatternStepState_t){0};
	}
}

OutputPatternEvent_t output_pattern_stop(OutputPattern_t *pattern, uint8_t output_state, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) {
		return snapshot(pattern);
	}

	bool was_running = (pattern->execution_state == PATTERN_EXEC_RUNNING);

	// On valide l'étape en cours pour figer le pattern à l'endroit exact de l'arrêt
	OutputPatternEvent_t event = was_running ? output_pattern_update(pattern, now_ms)
	                                         : snapshot(pattern);

	if (was_running) {
		// Le temps déjà écoulé dans l'étape est mémorisé dans step_start_ms
		OutputPatternStepState_t *state = &pattern->states[pattern->current_step_id];
		state->step_start_ms = (time_ms_t)(now_ms - state->step_start_ms);
	}

	pattern->execution_state = PATTERN_EXEC_STOPPED;
	event.pattern_state = PATTERN_EXEC_STOPPED;

	run_apply(pattern, output_state);

	return event;
}

OutputPatternEvent_t output_pattern_pause(OutputPattern_t *pattern, uint8_t output_state, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->execution_state != PATTERN_EXEC_RUNNING) {
		return snapshot(pattern);
	}

	// On valide l'étape en cours pour ne pas perdre une étape terminée à cet instant
	OutputPatternEvent_t event = output_pattern_update(pattern, now_ms);

	// Le pattern s'est terminé pendant la mise à jour : la pause n'a plus lieu d'être
	if (pattern->execution_state == PATTERN_EXEC_COMPLETED) return event;

	pattern->execution_state = PATTERN_EXEC_PAUSED;
	event.pattern_state = PATTERN_EXEC_PAUSED;

	run_apply(pattern, output_state);

	return event;
}

OutputPatternEvent_t output_pattern_resume(OutputPattern_t *pattern, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) {
		return snapshot(pattern);
	}

	if (output_pattern_is_completed(pattern)) {
		pattern->execution_state = PATTERN_EXEC_COMPLETED;
		return snapshot(pattern);
	}

	// Après un arrêt, step_start_ms contient le temps déjà écoulé : on le
	// repositionne par rapport à l'instant de reprise
	if (pattern->execution_state == PATTERN_EXEC_STOPPED) {
		OutputPatternStepState_t *state = &pattern->states[pattern->current_step_id];
		state->step_start_ms = (time_ms_t)(now_ms - state->step_start_ms);
	}

	pattern->execution_state = PATTERN_EXEC_RUNNING;

	run_apply(pattern, pattern->sequence->steps[pattern->current_step_id].value);

	return snapshot(pattern);
}

bool output_pattern_is_completed(const OutputPattern_t *pattern)
{
	if (pattern == NULL) return false;

	if (pattern->cycle_target == PATTERN_UNLIMITED_CYCLES) return false;

	return pattern->cycles_done >= pattern->cycle_target;
}

// Instantané de l'état d'exécution. Consomme les flags new_failed / new_skipped.
static OutputPatternEvent_t snapshot(OutputPattern_t *pattern)
{
	OutputPatternEvent_t event = {0};

	if (pattern == NULL) return event;

	event.pattern_state = pattern->execution_state;
	event.current_step = pattern->current_step_id;
	event.cycles_done = pattern->cycles_done;
	event.new_failed = pattern->flag_any_failed;
	event.new_skipped = pattern->flag_any_skipped;

	pattern->flag_any_failed = false;
	pattern->flag_any_skipped = false;

	return event;
}
