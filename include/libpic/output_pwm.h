/**
 * @file output_pwm.h
 * @brief Gestion d'une sortie PWM pilotée en pourcentage.
 *
 * Le module encapsule une sortie PWM matérielle derrière un rapport cyclique
 * exprimé en pourcentage (0 à OUTPUT_PWM_DUTY_MAX). La fréquence, la résolution
 * et la conversion pourcentage -> valeur de timer restent du ressort de la
 * fonction d'écriture fournie par l'application (write_pwm_fn) : le module ne
 * manipule que la valeur 0..100.
 *
 * L'application fournit la fonction d'écriture, comme pour output_t : le code
 * spécifique à la plateforme (registres PWM, timer) reste hors de la
 * bibliothèque.
 *
 * Cycle de vie :
 *
 *   - output_pwm_init() mémorise le rapport cyclique initial et ne l'écrit sur
 *     la sortie que si la PWM est créée activée ;
 *   - output_pwm_set_duty() mémorise et applique une nouvelle valeur ; 0 coupe
 *     la sortie, OUTPUT_PWM_DUTY_MAX l'active en permanence ;
 *   - output_pwm_disable() n'agit pas sur la sortie : le rapport cyclique
 *     courant reste appliqué, mais les changements de duty sont refusés. Pour
 *     forcer un état sûr, appeler output_pwm_set_duty(pwm, 0) avant ;
 *   - output_pwm_enable() réapplique le rapport cyclique mémorisé.
 *
 * Tant que la PWM est désactivée, output_pwm_set_duty() échoue et renvoie
 * false : le module garantit qu'aucune écriture matérielle ne survient.
 */
#ifndef LIBPIC_OUTPUT_PWM_H
#define LIBPIC_OUTPUT_PWM_H

/** Rapport cyclique maximal, en pourcentage (100 %). */
#define OUTPUT_PWM_DUTY_MAX 100

/** Valeur invalide renvoyée par output_pwm_get_duty() si le pointeur est NULL. */
#define OUTPUT_PWM_DUTY_INVALID 255

/**
 * Rapport cyclique sentinelle signifiant « ne pas modifier la sortie ».
 *
 * Hors de la plage 0..OUTPUT_PWM_DUTY_MAX, cette valeur ne peut jamais être un
 * rapport cyclique réel ; output_pwm_set_duty() la refuse. Elle est acceptée par
 * output_pwm_pattern_stop(), output_pwm_pattern_pause() et state_on_fail pour
 * laisser la sortie dans son état courant.
 */
#define OUTPUT_PWM_DUTY_KEEP 255

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente une sortie PWM pilotée en pourcentage. */
typedef struct {
    write_pwm_fn write;     /**< Fonction d'écriture de la PWM (fournie par l'application). */
    uint16_t output_pwm_id; /**< Identifiant de la PWM physique. */
    uint8_t duty_cycle;     /**< Rapport cyclique courant, en pourcentage (0 à 100). */
    bool enabled;           /**< true si la PWM accepte les changements de duty. */
} output_pwm_t;

/**
 * Initialise une sortie PWM.
 *
 * @param pwm           Sortie PWM à initialiser.
 * @param write         Fonction d'écriture de la PWM physique.
 * @param output_pwm_id Identifiant de la PWM physique.
 * @param initial_duty  Rapport cyclique initial, en pourcentage (0 à 100).
 * @param enabled       true pour activer la PWM dès l'initialisation.
 * @return true si les arguments sont valides, false sinon (pointeur NULL ou
 *         duty supérieur à OUTPUT_PWM_DUTY_MAX).
 *
 * @note Le rapport cyclique initial n'est écrit sur la sortie que si enabled
 *       vaut true ; sinon il est seulement mémorisé pour un futur
 *       output_pwm_enable().
 */
bool output_pwm_init(output_pwm_t *pwm,
                     write_pwm_fn write,
                     uint16_t output_pwm_id,
                     uint8_t initial_duty,
                     bool enabled);

/**
 * Applique un nouveau rapport cyclique.
 *
 * @param pwm  Sortie PWM à modifier.
 * @param duty Rapport cyclique, en pourcentage (0 à OUTPUT_PWM_DUTY_MAX).
 * @return true si la valeur a été appliquée, false si le pointeur ou la
 *         fonction d'écriture est NULL, si duty dépasse OUTPUT_PWM_DUTY_MAX ou
 *         si la PWM est désactivée.
 */
bool output_pwm_set_duty(output_pwm_t *pwm, uint8_t duty);

/**
 * Retourne le rapport cyclique mémorisé.
 *
 * @param pwm Sortie PWM à consulter.
 * @return Rapport cyclique courant (0 à OUTPUT_PWM_DUTY_MAX), ou
 *         OUTPUT_PWM_DUTY_INVALID si pwm est NULL ou si la valeur mémorisée
 *         est hors bornes (structure non initialisée).
 *
 * @note La valeur mémorisée est retournée même lorsque la PWM est désactivée.
 */
uint8_t output_pwm_get_duty(const output_pwm_t *pwm);

/**
 * Active la PWM et réapplique le rapport cyclique mémorisé.
 *
 * @param pwm Sortie PWM à activer.
 */
void output_pwm_enable(output_pwm_t *pwm);

/**
 * Désactive la PWM.
 *
 * @param pwm Sortie PWM à désactiver.
 *
 * @note La sortie n'est pas modifiée : le rapport cyclique courant reste
 *       appliqué par le matériel, seuls les changements de duty sont refusés.
 */
void output_pwm_disable(output_pwm_t *pwm);

/**
 * Indique si la PWM accepte les changements de rapport cyclique.
 *
 * @param pwm Sortie PWM à consulter.
 * @return true si la PWM est activée, false sinon (y compris si pwm est NULL).
 */
bool output_pwm_is_enabled(const output_pwm_t *pwm);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_OUTPUT_PWM_H */