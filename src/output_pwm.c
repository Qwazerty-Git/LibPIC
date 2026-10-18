/**
 * @file output_pwm.c
 * @brief Gestion d'une sortie PWM pilotée en pourcentage.
 */

#include "libpic/output_pwm.h"


bool output_pwm_init(output_pwm_t *pwm, write_pwm_fn write, uint16_t output_pwm_id, uint8_t initial_duty, bool enabled)
{
	if (pwm == NULL || write == NULL || initial_duty > OUTPUT_PWM_DUTY_MAX) return false;

	pwm->write = write;
	pwm->output_pwm_id = output_pwm_id;
	pwm->duty_cycle = initial_duty; // On mémorise le duty cycle initial, mais on ne l'applique pas encore si la PWM est désactivée
	pwm->enabled = enabled;
	output_pwm_set_duty(pwm, initial_duty);
	return true;
}

bool output_pwm_set_duty(output_pwm_t *pwm, uint8_t duty)
{
	if (pwm == NULL || pwm->write == NULL || duty > OUTPUT_PWM_DUTY_MAX || !pwm->enabled) return false;

	pwm->duty_cycle = duty;
	pwm->write(pwm->output_pwm_id, pwm->duty_cycle);
	return true;
}


uint8_t output_pwm_get_duty(const output_pwm_t *pwm)
{
	if (pwm == NULL || pwm->duty_cycle > OUTPUT_PWM_DUTY_MAX) return OUTPUT_PWM_DUTY_INVALID;

	return pwm->duty_cycle;
}


void output_pwm_enable(output_pwm_t *pwm)
{
	if (pwm == NULL) return;	

	pwm->enabled = true;
	output_pwm_set_duty(pwm, pwm->duty_cycle); // Réapplique le duty cycle actuel pour activer la PWM
}


void output_pwm_disable(output_pwm_t *pwm)
{
	if (pwm == NULL) return;	

	pwm->enabled = false;
}

bool output_pwm_is_enabled(const output_pwm_t *pwm)
{
	if (pwm == NULL) return false;

	return pwm->enabled;
}