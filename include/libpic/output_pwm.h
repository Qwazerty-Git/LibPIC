/**
 * @file pwm.h
 * @brief Gestion d'une sortie PWM.
 */
#ifndef LIBPIC_PWM_H
#define LIBPIC_PWM_H

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    write_pwm_fn write;
    uint16_t pwm_id;
    uint8_t duty_cycle;
    bool enabled;
} pwm_t;

void pwm_init(pwm_t *pwm,
              write_pwm_fn write,
              uint16_t pwm_id,
              uint8_t initial_duty);

void pwm_set_duty(pwm_t *pwm, uint8_t duty);
uint8_t pwm_get_duty(const pwm_t *pwm);

void pwm_enable(pwm_t *pwm);
void pwm_disable(pwm_t *pwm);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_PWM_H */