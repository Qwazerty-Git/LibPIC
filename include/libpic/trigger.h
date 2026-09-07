/**
 * @file trigger.h
 * @brief Gestion de déclenchements et d'événements sur des conditions arbitraires.
 */
#ifndef LIBPIC_TRIGGER_H
#define LIBPIC_TRIGGER_H

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fonction évaluant la condition surveillée par le déclencheur. */
typedef bool (*trigger_condition_fn)(void *context);

/** Fonction de rappel appelée lorsque le déclencheur se déclenche. */
typedef void (*trigger_callback_fn)(void *context);

/** Représente un déclenchement (trigger) surveillant une condition. */
typedef struct {
    trigger_condition_fn condition;  /**< Fonction évaluant la condition surveillée. */
    trigger_callback_fn callback;    /**< Fonction de rappel appelée au déclenchement. */
    void *context;                   /**< Contexte utilisateur transmis aux fonctions ci-dessus. */
    bool previous_state;             /**< Dernier état connu de la condition. */
    bool enabled;                    /**< true si le déclencheur est actif. */
} trigger_t;

/**
 * Initialise un déclencheur.
 *
 * @param trigger   Déclencheur à initialiser.
 * @param condition Fonction évaluant la condition surveillée (obligatoire).
 * @param callback  Fonction de rappel appelée au déclenchement (peut être NULL).
 * @param context   Contexte utilisateur transmis aux fonctions ci-dessus.
 */
void trigger_init(trigger_t *trigger, trigger_condition_fn condition, trigger_callback_fn callback, void *context);

/** Active ou désactive le déclencheur. */
void trigger_enable(trigger_t *trigger, bool enable);

/** Retourne true si le déclencheur est actif. */
bool trigger_is_enabled(const trigger_t *trigger);

/**
 * Évalue la condition et déclenche l'événement (front montant) si nécessaire.
 * Doit être appelée périodiquement.
 *
 * @param trigger Déclencheur à mettre à jour.
 * @return true si le déclencheur vient de se déclencher lors de cet appel.
 */
bool trigger_update(trigger_t *trigger);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_TRIGGER_H */
