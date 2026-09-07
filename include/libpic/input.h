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
    time_ms_t last_change_time;    /**< Instant du dernier changement d'état brut détecté. */
    time_ms_t last_call_time;      /**< Instant du dernier appel à input_update. */
    uint16_t pin_id;               /**< Identifiant de la broche physique (masque de bit). */
} input_t;

/** Événement d'entrée représentant un changement d'état d'une broche. */
typedef struct {
    bool rising_edge;
    bool falling_edge;
    bool reliable;
} input_event_t;

/**
 * Initialise une entrée.
 *
 * @param input             Entrée à initialiser.
 * @param read              Fonction de lecture de la broche physique.
 * @param pin_id            Identifiant de la broche physique (masque de bit).
 * @param active_high       true si l'entrée est active à l'état haut.
 * @param debounce_delay_ms Délai d'anti-rebond en millisecondes.
 */
void input_init(input_t *input, read_pin_fn read,uint16_t pin_id , bool active_high, time_ms_t debounce_delay_ms);

/**
 * Met à jour l'état de l'entrée. Doit être appelée périodiquement.
 *
 * @param input  Entrée à mettre à jour.
 * @param now_ms Instant courant en millisecondes.
 */
input_event_t input_update(input_t *input, time_ms_t now_ms);

/** Retourne true si l'entrée est actuellement active (stabilisée). */
bool input_is_active(const input_t *input);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_INPUT_H */
