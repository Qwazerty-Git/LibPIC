/**
 * @file output_group.h
 * @brief Gestion d'un groupe de sorties numériques.
 */
#ifndef LIBPIC_OUTPUT_GROUP_H
#define LIBPIC_OUTPUT_GROUP_H

#include "commun.h"
#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente un groupe de sorties, utile pour des opérations groupées. */
typedef struct {
    output_t **outputs;  /**< Tableau de pointeurs vers les sorties du groupe. */
    uint8_t count;        /**< Nombre de sorties dans le groupe. */
} output_group_t;

/**
 * Initialise un groupe de sorties.
 *
 * @param group   Groupe à initialiser.
 * @param outputs Tableau de pointeurs vers les sorties (doit rester valide).
 * @param count   Nombre de sorties du tableau.
 */
void output_group_init(output_group_t *group, output_t **outputs, uint8_t count);

/** Positionne l'état logique de toutes les sorties du groupe. */
void output_group_set_all(output_group_t *group, bool active);

/** Inverse l'état logique de toutes les sorties du groupe. */
void output_group_toggle_all(output_group_t *group);

/** Positionne l'état logique d'une sortie du groupe à l'index donné. */
void output_group_set_index(output_group_t *group, uint8_t index, bool active);

/** Retourne un pointeur vers la sortie à l'index donné, ou NULL si hors limites. */
output_t *output_group_get(const output_group_t *group, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_OUTPUT_GROUP_H */
