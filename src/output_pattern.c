/**
 * @file output_pattern.c
 * @brief Gestion de pattern sur une sortie.
 */

#include "libpic/output_pattern.h"

static OutputPatternEvent_t make_event(OutputPattern_t *pattern);
static void apply_output(OutputPattern_t *pattern, TargetState_t output_state);

// Initialisation
bool output_pattern_init(OutputPattern_t *pattern, output_t *output)
{
	if (pattern == NULL || output == NULL) return false;

	pattern->output = output;
	output_pattern_reset(pattern);
	return true;
}

// output_pattern : pointeur vers l'output_pattern à utiliser
// now_ms : temps actuel en millisecondes
OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms)
{
	OutputPatternEvent_t event = make_event(pattern);

	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) return event;

	// On regarde l'état d'execution du pattern
	if (pattern->execution_state == PATTERN_EXEC_ABORDED ||
		pattern->execution_state == PATTERN_EXEC_COMPLETED ||
		pattern->execution_state == PATTERN_EXEC_STOPPED) return event;

	// On pointe le step/state en cours
	OutputPatternStepState_t *current_state = &pattern->states[pattern->current_step_id];
	const OutputPatternStep_t *current_step = &pattern->sequence->steps[pattern->current_step_id];

	time_ms_t elapsed_ms = (time_ms_t)(now_ms - current_state->step_start_ms);

	// Step en cours non terminé : la sortie reste dans l'état du step
	if (elapsed_ms < current_step->duration_ms) {
		if (pattern->execution_state == PATTERN_EXEC_RUNNING) {
			apply_output(pattern, current_step->active ? OUTPUT_ON : OUTPUT_OFF);
		}
		return event;
	}

	// Écart entre la fin théorique du step et l'instant réel de validation
	time_ms_t overrun_ms = elapsed_ms - current_step->duration_ms;

	// Durée de la phase qui aurait dû démarrer à temps (0 si le pattern se termine)
	uint16_t next_duration_ms = 0;
	if (pattern->current_step_id + 1u < pattern->sequence->length) {
		next_duration_ms = pattern->sequence->steps[pattern->current_step_id + 1u].duration_ms;
	} else if (pattern->cycle_target == PATTERN_UNLIMITED_MS ||
	           pattern->cycles_done + 1u < pattern->cycle_target) {
		next_duration_ms = pattern->sequence->steps[0].duration_ms;
	}

	// Une phase entière (ou plus) a été avalée depuis le dernier appel.
	// Le contrat temporel (appeler update au moins une fois par durée d'étape)
	// vaut aussi en pause : seule la sortie y est figée.
	bool step_late = (next_duration_ms > 0u) && (overrun_ms >= next_duration_ms);

	//On valide le step
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

		apply_output(pattern, pattern->state_on_fail);
		return event;
	}

	//Dans les deux cas (running et paused), on pointe le step suivant
	if (pattern->current_step_id + 1u < pattern->sequence->length) {
		pattern->current_step_id++;
	} else {
		// On a terminé un cycle, on incrémente le compteur de cycles réalisés
		pattern->cycles_done++;

		// Dernier cycle réalisé -> pattern complet, la sortie conserve l'état de la dernière étape
		if (pattern->cycle_target != PATTERN_UNLIMITED_MS && pattern->cycles_done >= pattern->cycle_target) {
			pattern->execution_state = PATTERN_EXEC_COMPLETED;

			event.pattern_state = pattern->execution_state;
			event.current_step = pattern->current_step_id;
			event.cycles_done = pattern->cycles_done;

			return event;
		}

		//On repart sur un nouveau cycle, les états des steps repartent de zéro
		pattern->current_step_id = 0;

		for (uint8_t i = 0; i < pattern->sequence->length; ++i) {
			pattern->states[i].state = OUTPUT_STEP_PENDING;
			pattern->states[i].step_start_ms = 0;
		}
	}

	current_state = &pattern->states[pattern->current_step_id];
	current_step = &pattern->sequence->steps[pattern->current_step_id];

	current_state->state = OUTPUT_STEP_RUNNING;
	current_state->step_start_ms = now_ms;

	event.pattern_state = pattern->execution_state;
	event.current_step = pattern->current_step_id;
	event.cycles_done = pattern->cycles_done;

	// On applique l'état du nouveau step (sauf en pause)
	if (pattern->execution_state == PATTERN_EXEC_RUNNING) {
		apply_output(pattern, current_step->active ? OUTPUT_ON : OUTPUT_OFF);
	}

	return event;
}

// On transmet la séquence à exécuter
bool output_pattern_run(OutputPattern_t *pattern,
						const OutputPatternSequence_t *sequence,
						OutputPatternStepState_t *states,
						uint8_t states_count,
						uint16_t cycle_target,
						bool abort_on_fail,
						TargetState_t state_on_fail,
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

	//On active le pattern
	OutputPatternStepState_t *current_state = &pattern->states[pattern->current_step_id];
	const OutputPatternStep_t *current_step = &pattern->sequence->steps[pattern->current_step_id];

	pattern->execution_state = PATTERN_EXEC_RUNNING;
	current_state->state = OUTPUT_STEP_RUNNING;
	current_state->step_start_ms = now_ms;

	apply_output(pattern, current_step->active ? OUTPUT_ON : OUTPUT_OFF);

	return true;
}

// Reset : réinitialise tous les step de la séquence en cours (steps en pending) + mise en pause
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
		pattern->states[i].state = OUTPUT_STEP_PENDING;
		pattern->states[i].step_start_ms = 0;
	}
}

// Pause / Reprise

OutputPatternEvent_t output_pattern_stop(OutputPattern_t *pattern, TargetState_t output_state, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) {
		return make_event(pattern);
	}

	bool was_running = (pattern->execution_state == PATTERN_EXEC_RUNNING);

	//On appelle update pour valider la position où doit être figée le pattern
	OutputPatternEvent_t event = was_running ? output_pattern_update(pattern, now_ms) : make_event(pattern);

	if (was_running) {
		//On utilise le state du step actuel pour mémoriser le temps déjà écoulé dans le step
		OutputPatternStepState_t *state = &pattern->states[pattern->current_step_id];
		state->step_start_ms = (time_ms_t)(now_ms - state->step_start_ms);
	}

	pattern->execution_state = PATTERN_EXEC_STOPPED;
	event.pattern_state = PATTERN_EXEC_STOPPED;

	apply_output(pattern, output_state);

	return event;
}

OutputPatternEvent_t output_pattern_pause(OutputPattern_t *pattern, TargetState_t output_state, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->execution_state != PATTERN_EXEC_RUNNING) {
		return make_event(pattern);
	}

	// On déclenche un update pour ne pas skipper un step qui serait terminé au moment de l'appel à pause
	OutputPatternEvent_t event = output_pattern_update(pattern, now_ms);

	// Le pattern s'est terminé pendant l'update, la pause n'a plus lieu d'être
	if (pattern->execution_state == PATTERN_EXEC_COMPLETED) return event;

	pattern->execution_state = PATTERN_EXEC_PAUSED;
	event.pattern_state = PATTERN_EXEC_PAUSED;

	apply_output(pattern, output_state);

	return event;
}

OutputPatternEvent_t output_pattern_resume(OutputPattern_t *pattern, time_ms_t now_ms)
{
	if (pattern == NULL || pattern->sequence == NULL || pattern->states == NULL ||
		pattern->current_step_id >= pattern->sequence->length) {
		return make_event(pattern);
	}

	if (output_pattern_is_completed(pattern))
	{
		pattern->execution_state = PATTERN_EXEC_COMPLETED;
		return make_event(pattern);
	}

	// Si le pattern était en stopped, on doit mettre à jour le timer du stepState en cours
	// On récupère le temps déjà écoulé (et sauvegardé) dans le step_start_ms du state du step
	if (pattern->execution_state == PATTERN_EXEC_STOPPED) {
		OutputPatternStepState_t *state = &pattern->states[pattern->current_step_id];
		state->step_start_ms = (time_ms_t)(now_ms - state->step_start_ms);
	}

	pattern->execution_state = PATTERN_EXEC_RUNNING;

	OutputPatternEvent_t event = make_event(pattern);

	apply_output(pattern, pattern->sequence->steps[pattern->current_step_id].active ? OUTPUT_ON : OUTPUT_OFF);

	return event;
}

// Indique si le pattern est terminé
bool output_pattern_is_completed(const OutputPattern_t *pattern)
{
	if (pattern == NULL) return false;

	if (pattern->cycle_target == PATTERN_UNLIMITED_MS)
		return false; // Cycles illimités

	return pattern->cycles_done >= pattern->cycle_target;
}

// Instantané de l'état d'exécution. Consomme les flags new_failed / new_skipped.
static OutputPatternEvent_t make_event(OutputPattern_t *pattern)
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

// Applique un état cible à la sortie associée. OUTPUT_KEEP ne fait rien.
static void apply_output(OutputPattern_t *pattern, TargetState_t output_state)
{
	if (pattern == NULL || pattern->output == NULL || output_state == OUTPUT_KEEP) return;

	output_set(pattern->output, output_state == OUTPUT_ON);
}
