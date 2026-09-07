/*
trigger.h

Créé le: 22/08/2026
Créé par: Qwazerty

But : 
    Fournit une interface pour gérer des déclencheurs (triggers) basés sur des cycles,
    des motifs (patterns) ou des états synchronisés.
    Le trigger encapsule un Timer_t et ajoute une couche logique pour interpréter
    les évènements du timer.

Modes disponibles :

    - TRIGGER_CYCLE : 
        Déclenche un évènement à chaque période configurée.
        Utile pour des actions périodiques (ex: clignotement d'une LED).

    - TRIGGER_PATTERN : 
        Déclenche des évènements selon une séquence de durées (pattern).
        Chaque évènement passe au pattern suivant, puis boucle.
        Exemple : pattern = [100, 200, 300] → LED qui accélère puis ralentit.

    - TRIGGER_SYNC : 
        Déclenche un évènement quand un état externe change (ex: entrée GPIO).
        Permet de synchroniser plusieurs triggers sur un même signal.
        Il ne compte pas les cycles, il ne fait que détecter les changements d'état.
        Si l'état change entre deux appels à trigger_update(), un évènement est généré.
        C'est un déclencheur "front" (montant ou descendant) basé sur une lecture périodique.
        Si plusieurs changements ont lieu entre deux appels à trigger_update, il ne seront pas pris en compte



Fonctionnement :
    Le trigger est mis à jour via trigger_update(), qui transmet les ticks au timer interne.
    Quand un évènement est généré, le trigger l'interprète selon le mode configuré.
    Un callback optionnel peut être enregistré pour recevoir les évènements.

Méthodes exposées :

    - trigger_create : 
        Initialise un trigger à zéro. Doit être appelé avant toute autre fonction.

    - trigger_init_cycle : 
        Configure le trigger en mode CYCLE avec une période en ticks et un nombre de cycles.
        nbr_cycles = 0 signifie infini.

    - trigger_init_pattern : 
        Configure le trigger en mode PATTERN avec un TriggerPatternList_t (tableau de durées).
        nbr_cycles = 0 signifie infini.

    - trigger_init_sync : 
        Initialise le déclencheur pour un mode synchronisé avec un état externe.
        Permet de synchroniser plusieurs timers sur un même état externe.
        Le trigger surveille les changements d'état du pointeur *sync_state.
        À chaque appel de trigger_update(), l'état est comparé à la valeur précédente.
        Si un changement est détecté, un évènement est généré.
        NOTE : Ce mode est sensible à la fréquence d'appel de trigger_update().
                Si l'état change plus vite que la fréquence d'échantillonnage,
                certains changements peuvent être manqués.

    - trigger_update : 
        Met à jour le trigger avec les ticks écoulés. Retourne TriggerEvent_t.

    - trigger_set_receiver : 
        Enregistre un callback qui sera appelé quand un évènement est généré.

    - trigger_reset : 
        Remet à zéro le trigger (pattern_index, timer).

    - trigger_pause : 
        Met le trigger en pause.

    - trigger_resume : 
        Reprend le trigger après une pause.

    - trigger_get_mode : 
        Retourne le mode actuel du trigger.

    - trigger_get_pattern_index : 
        Retourne l'index actuel dans le pattern.

    - trigger_is_paused : 
        Indique si le trigger est en pause.

    - trigger_cycle_over : 
        Indique si les cycles configurés sont terminés.

    - trigger_is_sync : 
        Indique si le trigger est en mode SYNC et actif.

Note : 
    Les fonctions d'initialisation vérifient les paramètres et retournent false en cas d'erreur.
    En cas d'erreur, le mode est défini sur TRIGGER_NONE.

Exemple d'utilisation (mode CYCLE) :
    Trigger_t trigger;
    trigger_create(&trigger);
    trigger_init_cycle(&trigger, 1000, 0);  // toutes les 1000 ticks, infini

    // Dans la boucle principale :
    TriggerEvent_t event = trigger_update(&trigger, ticks_ecoules);
    if (event.event) {
        // Faire une action
    }
*/

#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdint.h>
#include <stdbool.h>
#include "timer.h"

typedef enum
{
    TRIGGER_NONE = 0,
    TRIGGER_CYCLE,
    TRIGGER_PATTERN,
    TRIGGER_SYNC
} TriggerMode;

// Structure de retour de la fonction trigger_update
typedef struct
{
    bool event;              // Indique si un évènement s'est produit
    uint32_t iteration;      // Numéro de l'itération actuelle
    uint16_t pattern_index;  // Index actuel dans le pattern (mode PATTERN)
    bool iteration_overflow; // Indique si le compteur d'itération a dépassé uint32_t
    bool cycle_over;         // Indique si les cycles configurés sont terminés
} TriggerEvent_t;

// Définition du callback utilisateur
typedef void (*TriggerReceiver)(void *context, const TriggerEvent_t trigger_e);

// Structure représentant un pattern (liste de durées)
typedef struct
{
    uint16_t *values;        // Tableau de durées en ticks
    uint16_t length;         // Nombre d'éléments dans le tableau
} TriggerPatternList_t;

// Structure principale du trigger
typedef struct
{
    // Variables privées, utiliser les fonctions d'accès
    TriggerMode mode;                // Mode actuel (CYCLE, PATTERN, SYNC)

    const bool *sync_state;          // Pointeur vers l'état externe à surveiller
    const TriggerPatternList_t *pattern;    // Pointeur vers le pattern (mode PATTERN)
    uint16_t pattern_index;          // Index actuel dans le pattern

    Timer_t timer;                   // Timer interne
    bool sync_last_state;            // Dernier état connu du signal sync

    uint16_t cycle_count;      // ← Nouveau : cycles restants (0 = infini)
    uint16_t cycle_max;        // ← Nouveau : nombre de cycles demandés (0 = infini)

    TriggerReceiver receiver;        // Callback optionnel
    void *receiver_context;          // Contexte passé au callback
} Trigger_t;

// Fonctions de gestion du cycle de vie
bool trigger_create(Trigger_t *trigger);
void trigger_set_receiver(Trigger_t *trigger, TriggerReceiver receiver, void *context);

// Fonctions d'initialisation
bool trigger_init_cycle(Trigger_t *trigger, uint16_t period, uint16_t nbr_cycles);
bool trigger_init_pattern(Trigger_t *trigger, const TriggerPatternList_t *pattern, uint16_t nbr_cycles);
bool trigger_init_sync(Trigger_t *trigger, const bool *sync_state);

// Fonctions d'accès
TriggerMode trigger_get_mode(const Trigger_t *trigger);
uint16_t trigger_get_pattern_index(const Trigger_t *trigger);
bool trigger_is_paused(const Trigger_t *trigger);
bool trigger_cycle_over(const Trigger_t *trigger);
bool trigger_is_sync(const Trigger_t *trigger);

// Fonctions de contrôle
TriggerEvent_t trigger_update(Trigger_t *trigger, uint16_t ticks);
void trigger_reset(Trigger_t *trigger);
void trigger_pause(Trigger_t *trigger);
TriggerEvent_t trigger_resume(Trigger_t *trigger);

#endif