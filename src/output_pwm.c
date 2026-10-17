/**
 * @file pwm.c
 * @brief Gestion d'une sortie PWM.
 */

#include "libpic/output_pwm.h"


void pwm_init(pwm_t *pwm, write_pwm_fn write, uint16_t pwm_id, uint8_t initial_duty)
{
	if (pwm == NULL || write == NULL) return;

	pwm->write = write;
	pwm->pwm_id = pwm_id;
	pwm->duty_cycle = initial_duty;
	//enable=false? true? inchangé? ajouter un param ?
}

void pwm_set_duty(pwm_t *pwm, uint8_t duty)
{
	if (pwm == NULL) return;

	pwm->duty_cycle = duty;
}


uint8_t pwm_get_duty(const pwm_t *pwm)
{
	if (pwm == NULL) return 255;

	return pwm->duty_cycle;
}


void pwm_enable(pwm_t *pwm)
{
	if (pwm == NULL) return;	

	pwm->enabled = true;
}


void pwm_disable(pwm_t *pwm)
{
	if (pwm == NULL) return;	

	pwm->enabled = false;
}
