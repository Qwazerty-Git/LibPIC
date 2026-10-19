/**
 * @file output_pwm_pattern.c
 * @brief Pattern et rampe sur une sortie PWM : adaptateur du noyau commun.
 *
 * Seuls init(), run_ramp(), update(), run() et reset() sont propres à la PWM ;
 * stop(), pause(), resume() et is_completed() sont des alias directs de
 * output_pattern_* déclarés dans l'en-tête.
 */

#include "libpic/output_pwm_pattern.h"

/* Adaptateur PWM : la consigne du noyau est un rapport cyclique. */
static void apply_pwm(void *output, uint8_t value)
{
	output_pwm_set_duty((output_pwm_t *)output, value);
}

/* Rapport cyclique courant de la sortie pilotée. */
static uint8_t current_duty(const OutputPwmPattern_t *pattern)
{
	return output_pwm_get_duty((const output_pwm_t *)pattern->base.output);
}

/* Prochaine consigne : avance de step_pct sans dépasser la cible. */
static uint8_t next_duty(uint8_t current, uint8_t target, uint8_t step_pct)
{
	if (target > current) {
		uint8_t value = (uint8_t)(current + step_pct);
		return (value > target) ? target : value;
	}

	if (current <= target) return target;

	uint8_t value = (current > step_pct) ? (uint8_t)(current - step_pct) : 0u;
	return (value < target) ? target : value;
}

/* Prépare et démarre l'étape interne de la rampe. */
static bool ramp_start_step(OutputPwmPattern_t *pattern, uint8_t value, time_ms_t step_ms, time_ms_t now_ms)
{
	pattern->ramp_step.value = value;
	pattern->ramp_step.duration_ms = step_ms;

	return output_pattern_run(&pattern->base, &pattern->ramp_sequence, &pattern->ramp_state,
	                          1u, 1u, false, OUTPUT_KEEP, now_ms);
}

bool output_pwm_pattern_init(OutputPwmPattern_t *pattern, output_pwm_t *output)
{
	if (pattern == NULL || output == NULL) return false;

	pattern->base.output = output;
	pattern->base.apply = apply_pwm;

	pattern->ramp_sequence.steps = &pattern->ramp_step;
	pattern->ramp_sequence.length = 1u;
	pattern->ramp_step_pct = 0u;

	output_pattern_reset(&pattern->base);

	return true;
}

OutputPatternEvent_t output_pwm_pattern_update(OutputPwmPattern_t *pattern, time_ms_t now_ms)
{
	if (pattern == NULL) return output_pattern_update(NULL, now_ms);

	// En pause, la rampe est figée : le palier est repoussé à chaque appel, donc
	// jamais consommé (le noyau ne valide une étape que si sa durée est écoulée)
	if (pattern->ramp_step_pct != 0u && pattern->base.execution_state == PATTERN_EXEC_PAUSED) {
		pattern->ramp_state.step_start_ms = now_ms;
		return output_pattern_update(&pattern->base, now_ms);
	}

	OutputPatternEvent_t event = output_pattern_update(&pattern->base, now_ms);

	if (pattern->ramp_step_pct == 0u) return event;

	// Palier encore en cours, en pause ou à l'arrêt : rien à ré-armer
	if (pattern->base.execution_state == PATTERN_EXEC_RUNNING ||
	    pattern->base.execution_state == PATTERN_EXEC_STOPPED) return event;

	if (pattern->base.execution_state == PATTERN_EXEC_ABORDED) {
		pattern->ramp_step_pct = 0u;
		return event;
	}

	// Palier atteint : on enchaîne jusqu'à la cible
	uint8_t current = current_duty(pattern);

	if (current == OUTPUT_PWM_DUTY_INVALID || current == pattern->ramp_target) {
		pattern->ramp_step_pct = 0u;
		return event;
	}

	if (ramp_start_step(pattern, next_duty(current, pattern->ramp_target, pattern->ramp_step_pct),
	                    pattern->ramp_step.duration_ms, now_ms)) {
		event.pattern_state = PATTERN_EXEC_RUNNING;
		event.current_step = 0u;
		event.cycles_done = 0u;
	} else {
		pattern->ramp_step_pct = 0u;
	}

	return event;
}

bool output_pwm_pattern_run(OutputPwmPattern_t *pattern,
                        const OutputPwmPatternSequence_t *sequence,
                        OutputPatternStepState_t *states,
                        uint8_t states_count,
                        uint8_t cycle_target,
                        bool abort_on_fail,
                        uint8_t state_on_fail,
                        time_ms_t now_ms)
{
	if (pattern == NULL) return false;

	// La séquence prend le contrôle de la sortie : la rampe est abandonnée
	pattern->ramp_step_pct = 0u;

	return output_pattern_run(&pattern->base, sequence, states, states_count,
	                          cycle_target, abort_on_fail, state_on_fail, now_ms);
}

bool output_pwm_pattern_run_ramp(OutputPwmPattern_t *pattern,
                        uint8_t target_duty,
                        uint8_t step_pct,
                        time_ms_t step_ms,
                        time_ms_t now_ms)
{
	if (pattern == NULL ||
		target_duty > OUTPUT_PWM_DUTY_MAX ||
		step_pct == 0u ||
		step_pct > OUTPUT_PWM_DUTY_MAX ||
		step_ms == 0u)
	{
		return false;
	}

	pattern->ramp_target = target_duty;

	uint8_t current = current_duty(pattern);

	// Déjà sur la cible : rien à faire, aucune rampe n'est laissée active
	if (current == target_duty) {
		pattern->ramp_step_pct = 0u;
		return true;
	}

	pattern->ramp_step_pct = step_pct;

	return ramp_start_step(pattern, next_duty(current, target_duty, step_pct), step_ms, now_ms);
}

void output_pwm_pattern_reset(OutputPwmPattern_t *pattern)
{
	if (pattern == NULL) return;

	pattern->ramp_step_pct = 0u;

	output_pattern_reset(&pattern->base);
}


