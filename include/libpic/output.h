/**
 * @file output.h
 * @brief Gestion d'une sortie numérique.
 */
#ifndef LIBPIC_OUTPUT_H
#define LIBPIC_OUTPUT_H

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente une sortie numérique unique. */
typedef struct {
    write_pin_fn write;  /**< Fonction d'écriture de la broche physique. */
    bool active_high;    /**< true si l'état actif correspond à STATE_HIGH. */
    bool state;          /**< État logique courant (true = actif). */
    uint16_t pin_id;      /**< Identifiant de la broche physique. */
} output_t;

/**
 * Initialise une sortie.
 *
 * @param output      Sortie à initialiser.
 * @param write       Fonction d'écriture de la broche physique.
 * @param id_pin      Identifiant de la broche physique.
 * @param active_high true si la sortie est active à l'état haut.
 * @param initial_state État initial de la sortie (true = actif).
 */
void output_init(output_t *output, write_pin_fn write,uint16_t id_pin, bool active_high, bool initial_state);

/** Positionne l'état logique de la sortie (true = actif). */
void output_set(output_t *output, bool active);

void output_off(output_t *output);
void output_on(output_t *output);

/** Inverse l'état logique de la sortie. */
void output_toggle(output_t *output);

/** Retourne true si la sortie est actuellement active. */
bool output_is_active(const output_t *output);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_OUTPUT_H */
