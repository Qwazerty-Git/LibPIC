/*
input_pattern.h

Créé le: 19/08/2026
Créé par: Qwazerty

But :
    Fournit une interface pour détecter des séquences d'appuis (patterns) sur une entrée.
    Ce module se positionne au-dessus de input.h et analyse les changements d'état
    validés par le debounce pour reconnaître des formes d'interaction utilisateur
    (clics, double-clics, clics longs, etc.).

Principe de fonctionnement :
    - L'utilisateur définit des patterns sous forme de séquences de durées maximales.
    - Chaque pattern est une alternance d'états : appui (1) et relâchement (0).
    - Les étapes paires (0, 2, 4...) représentent un appui (état 1).
    - Les étapes impaires (1, 3, 5...) représentent un relâchement (état 0).
    - Le module observe l'état de l'input et valide ou invalide chaque pattern
        en fonction des durées observées.

Exemple de patterns :

    // Clic simple : appui < 500ms
    static const uint16_t clic_steps[] = {500};

    // Clic long : appui >= 500ms et < 2000ms
    static const uint16_t clic_long_steps[] = {2000};

    // Double-clic : appui < 300ms, relâche < 300ms, appui < 300ms
    static const uint16_t double_clic_steps[] = {300, 300, 300};

    static const MatcherPattern_t patterns[] = {
        {clic_steps, 1, 0},           // ID 0 = clic simple
        {clic_long_steps, 1, 1},      // ID 1 = clic long
        {double_clic_steps, 3, 2},    // ID 2 = double-clic
    };

    static const MatcherPatternList_t pattern_list = {
        patterns, 3
    };

Utilisation :

    // Déclaration
    MatcherPatternState_t states[3];       // Un état par pattern (en RAM)
    MatcherPatternGlobal_t matcher;

    // Initialisation
    pattern_matcher_init(&matcher, &pattern_list, states, false);

    // Dans la boucle principale (toutes les 1ms)
    MatcherPatternResult_t result = pattern_matcher_update(&matcher, &bouton, 1);

    // Traitement des résultats
    if (result.any_matched) {
        if (pattern_matcher_is_matched(&matcher, 0)) {   // ID 0 = clic
            // Action pour un clic
        }
        if (pattern_matcher_is_matched(&matcher, 2)) {   // ID 2 = double-clic
            // Action pour un double-clic
        }
        // Reset après traitement (pour repartir propre)
        pattern_matcher_reset(&matcher);
    }
*/

#ifndef INPUT_PATTERN_H
#define INPUT_PATTERN_H

#include <stdbool.h>
#include <stdint.h>
#include "input.h"

// // Durée maximale d'une étape du pattern
// typedef struct {
//     uint16_t max_ticks;   // Durée maximale de l'étape (en ticks)
// } PatternStep_t;

// Un pattern = une séquence d'étapes
typedef struct {
    const uint16_t *steps;   // Tableau des étapes (en Flash)
    uint8_t length;               // Nombre d'étapes
    uint8_t id;                   // Identifiant pour l'utilisateur
} MatcherPattern_t;

// Liste de patterns
typedef struct {
    const MatcherPattern_t *patterns;    // Tableau de patterns (en Flash)
    uint8_t count;                // Nombre de patterns
} MatcherPatternList_t;

// État interne d'un pattern (en RAM)
typedef struct {
    uint8_t step_index;      // Étape actuelle (0 = première)
    uint16_t step_ticks;     // Temps écoulé dans l'étape actuelle
    bool active;             // Pattern encore possible
    bool matched;            // Pattern validé lors du dernier update
} MatcherPatternState_t;

// Résultat retourné par pattern_matcher_update
typedef struct {
    bool any_matched;   // true si au moins un pattern a été validé
} MatcherPatternResult_t;

// Structure principale du matcher
typedef struct {
    const MatcherPatternList_t *list;      // Liste des patterns (définition)
    MatcherPatternState_t *states;         // États des patterns (fourni par l'utilisateur)
    Input_t *input;                 // ← Le matcher connaît SON input
    bool paused;                    // true = ignore les ticks
} MatcherPatternGlobal_t;

// Initialisation du matcher
void pattern_matcher_init(MatcherPatternGlobal_t *matcher,
                        Input_t *input,               
                        const MatcherPatternList_t *pattern_list,
                        MatcherPatternState_t *states,
                        bool paused);

// Mise à jour : analyse les patterns en fonction de l'état de l'input
// input : pointeur vers l'input à surveiller
// ticks : nombre de ticks écoulés
// Retourne any_matched = true si au moins un pattern a été validé
MatcherPatternResult_t pattern_matcher_update(MatcherPatternGlobal_t *matcher,
                                        uint16_t ticks);

// Reset : réinitialise tous les états (tous les patterns redeviennent actifs)
void pattern_matcher_reset(MatcherPatternGlobal_t *matcher);

// Pause / Reprise
void pattern_matcher_pause(MatcherPatternGlobal_t *matcher);
void pattern_matcher_resume(MatcherPatternGlobal_t *matcher);

// Indique si tous les patterns sont invalidés
bool pattern_matcher_all_inactive(const MatcherPatternGlobal_t *matcher);

// Indique si un pattern spécifique (par ID) a été validé
bool pattern_matcher_is_matched(const MatcherPatternGlobal_t *matcher, uint8_t id);

#endif