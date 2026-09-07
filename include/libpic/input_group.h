/**
 * @file input_group.h
 * @brief Gestion d'un groupe d'entrées numériques.
 */
#ifndef LIBPIC_INPUT_GROUP_H
#define LIBPIC_INPUT_GROUP_H

#include "commun.h"
#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente un groupe d'entrées, utile pour des opérations groupées. */
typedef struct {
    input_t **inputs;  /**< Tableau de pointeurs vers les entrées du groupe. */
    uint8_t count;      /**< Nombre d'entrées dans le groupe. */
} input_group_t;

/**
 * Initialise un groupe d'entrées.
 *
 * @param group  Groupe à initialiser.
 * @param inputs Tableau de pointeurs vers les entrées (doit rester valide).
 * @param count  Nombre d'entrées du tableau.
 */
void input_group_init(input_group_t *group, input_t **inputs, uint8_t count);

/** Met à jour toutes les entrées du groupe. */
void input_group_update(input_group_t *group, time_ms_t now_ms);

/** Retourne true si au moins une entrée du groupe est active. */
bool input_group_any_active(const input_group_t *group);

/** Retourne true si toutes les entrées du groupe sont actives. */
bool input_group_all_active(const input_group_t *group);

/** Retourne le nombre d'entrées actives dans le groupe. */
uint8_t input_group_active_count(const input_group_t *group);

/** Retourne un pointeur vers l'entrée à l'index donné, ou NULL si hors limites. */
input_t *input_group_get(const input_group_t *group, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_INPUT_GROUP_H */
