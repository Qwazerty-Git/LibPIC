/**
 * @file commun.h
 * @brief Structures et types communs partagés par les composants de LibPIC.
 */
#ifndef LIBPIC_COMMUN_H
#define LIBPIC_COMMUN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Valeur cible d'une sortie.
 *
 * OUTPUT_KEEP est volontairement hors de la plage d'un rapport cyclique
 * (0 à 100) : une consigne de 2 % ne peut donc jamais être confondue avec
 * « ne pas modifier la sortie ». Les modules de pattern utilisent cette même
 * valeur comme sentinelle interne.
 */
typedef enum {
    OUTPUT_OFF = 0,
    OUTPUT_ON = 1,
    OUTPUT_KEEP = 255
} TargetState_t;

/**
 * Durée absente : l'étape de input_pattern n'a pas de durée maximale et attend
 * l'événement.
 */
#define PATTERN_UNLIMITED_MS 0

/**
 * Nombre de cycles illimité : cycle_target de output_pattern et de
 * output_pwm_pattern.
 */
#define PATTERN_UNLIMITED_CYCLES 0

typedef enum {
    PATTERN_EXEC_STOPPED,
    PATTERN_EXEC_PAUSED,
    PATTERN_EXEC_RUNNING,
    PATTERN_EXEC_COMPLETED,
    PATTERN_EXEC_ABORDED
} PatternExecutionState_t;

/** État logique d'une broche (entrée ou sortie). */
typedef enum {
    STATE_LOW = 0,
    STATE_HIGH = 1
} state_t;

/**
 * Type utilisé pour représenter un instant ou une durée, en millisecondes.
 *
 * Sur 16 bits, la plage est de 65 535 ms. Les calculs de durée des modules se
 * font par soustraction modulaire : ils restent exacts tant que l'intervalle
 * réel entre deux appels d'un module reste inférieur à 65,5 s. Au-delà, l'écart
 * est interprété modulo 65 536 et devient faux. Une pause ou un arrêt long
 * n'est pas concerné : seul compte l'intervalle entre deux mises à jour.
 */
typedef uint16_t time_ms_t;

/** Fonction de lecture d'une broche physique (à fournir par le projet appelant). */
typedef state_t (*read_pin_fn)(uint16_t pin_id);

/** Fonction d'écriture d'une broche physique (à fournir par le projet appelant). */
typedef void (*write_pin_fn)(uint16_t pin_id, state_t state);

/** Fonction d'écriture d'une broche pwm (à fournir par le programme appelant). */
typedef void(*write_pwm_fn)(uint16_t pin_id, uint8_t  duty_cycle);

/** États possibles d'une étape durant l'exécution d'un output_(pwm)_pattern. */
typedef enum {
    OUTPUT_STEP_PENDING = 0x00u,     /**< Pas encore commencée. */
    OUTPUT_STEP_RUNNING = 0x01u,     /**< En cours. */
    OUTPUT_STEP_COMPLETED = 0x02u,   /**< Réalisée normalement. */
    OUTPUT_STEP_SKIPPED = 0x03u,     /**< Passée sans être exécutée. */
    OUTPUT_STEP_FAILED = 0x04u       /**< Échec : dépassement de durée trop important (voir @file). */
} OutputPatternStepStateEnum_t;

/**
 * État interne d'une étape (en RAM, fourni par l'utilisateur).
 * Identique pour output_pattern et output_pwm_pattern.
 */
typedef struct {
    time_ms_t step_start_ms;                   /**< Temps de début de l'étape actuelle. */
    OutputPatternStepStateEnum_t state;        /**< État actuel de l'étape. */
} OutputPatternStepState_t;

/** Résultat retourné par output_(pwm)_pattern */
typedef struct {
    PatternExecutionState_t pattern_state;   /**< État d'exécution du pattern. */
    uint8_t current_step;                    /**< Index de l'étape en cours (0 = première). */
    uint8_t cycles_done;                     /**< Nombre de cycles réalisés (modulo 256). */
    bool new_failed;                         /**< true si au moins une étape a échoué depuis l'appel précédent. */
    bool new_skipped;                        /**< true si au moins une étape a été passée depuis l'appel précédent. */
} OutputPatternEvent_t;



#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_COMMUN_H */
