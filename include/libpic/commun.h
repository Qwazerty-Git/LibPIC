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

/** Valeur cible d'une sortie. Keep : conserve l'état actuel */
typedef enum {
    OUTPUT_OFF,
    OUTPUT_ON,
    OUTPUT_KEEP
} TargetState_t;

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

/** Type utilisé pour représenter un instant ou une durée, en millisecondes. */
typedef uint32_t time_ms_t;

/** Fonction de lecture d'une broche physique (à fournir par le projet appelant). */
typedef state_t (*read_pin_fn)(uint16_t pin_id);

/** Fonction d'écriture d'une broche physique (à fournir par le projet appelant). */
typedef void (*write_pin_fn)(uint16_t pin_id, state_t state);

/** Fonction d'écriture d'une broche pwm (à fournir pas le programme appelant). */
typedef void(*write_pwm_fn)(uint16_t pin_id, uint8_t  duty_cycle);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_COMMUN_H */
