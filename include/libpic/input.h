/**
 * @file input.h
 * @brief Gestion d'une entrée numérique avec anti-rebond (debounce).
 */
#ifndef LIBPIC_INPUT_H
#define LIBPIC_INPUT_H

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente une entrée numérique unique. */
typedef struct {
    read_pin_fn read;              /**< Fonction de lecture de la broche physique. */
    bool active_high;              /**< true si l'état actif correspond à STATE_HIGH. */
    time_ms_t debounce_delay_ms;   /**< Délai d'anti-rebond, en millisecondes. */
    state_t raw_state;             /**< Dernier état brut lu sur la broche. */
    bool state;                    /**< État logique stabilisé (true = actif). */
    bool previous_state;           /**< État logique stabilisé lors du précédent update. */
    time_ms_t last_change_time;    /**< Instant du dernier changement d'état brut détecté. */
} input_t;

/**
 * Initialise une entrée.
 *
 * @param input             Entrée à initialiser.
 * @param read              Fonction de lecture de la broche physique.
 * @param active_high       true si l'entrée est active à l'état haut.
 * @param debounce_delay_ms Délai d'anti-rebond en millisecondes.
 */
void input_init(input_t *input, read_pin_fn read, bool active_high, time_ms_t debounce_delay_ms);

/**
 * Met à jour l'état de l'entrée. Doit être appelée périodiquement.
 *
 * @param input  Entrée à mettre à jour.
 * @param now_ms Instant courant en millisecondes.
 */
void input_update(input_t *input, time_ms_t now_ms);

/** Retourne true si l'entrée est actuellement active (stabilisée). */
bool input_is_active(const input_t *input);

/** Retourne true si l'entrée vient de passer de inactive à active. */
bool input_rising_edge(const input_t *input);

/** Retourne true si l'entrée vient de passer de active à inactive. */
bool input_falling_edge(const input_t *input);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_INPUT_H */
