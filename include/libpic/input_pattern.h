/**
 * @file input_pattern.h
 * @brief Reconnaissance de séquences d'états sur une entrée numérique.
 *
 * Ce module observe une entrée input_t et reconnaît des séquences
 * d'appuis et de relâchements après validation par l'anti-rebond.
 *
 * Une séquence est décrite par un tableau de PatternStep_t.
 * Chaque étape correspond à un état logique de l'entrée :
 *
 *   - étape paire  (0, 2, 4, ...) : entrée active, appui ;
 *   - étape impaire (1, 3, 5, ...) : entrée inactive, relâchement.
 *
 * Les événements sont logiques et indépendants du niveau électrique de la
 * broche. Un bouton actif-bas produit donc normalement un rising_edge lors
 * de l'appui physique, car l'état logique passe de inactif à actif.
 *
 * Pour chaque étape :
 *
 *   - min_ms est la durée minimale exigée ;
 *   - max_ms est la durée maximale autorisée ;
 *   - max_ms == PATTERN_UNLIMITED_MS signifie qu'il n'y a pas de durée
 *     maximale.
 *
 * Une valeur max_ms non nulle et inférieure à min_ms constitue une erreur
 * de configuration.
 *
 * Le temps de chaque étape est mesuré depuis son début avec un horodatage
 * absolu en millisecondes. Les valeurs now_ms doivent provenir du même
 * compteur monotone que celui utilisé par input_update().
 *
 * Le premier rising_edge démarre la première étape. Il ne termine pas cette
 * étape et ne déclenche donc pas encore le contrôle de min_ms.
 *
 * Lorsqu'un événement logique survient :
 *
 *   - rising_edge est attendu pour une étape d'appui ;
 *   - falling_edge est attendu pour une étape de relâchement.
 *
 * Un événement inattendu ou non fiable invalide le pattern courant, mais ne
 * constitue pas une erreur de configuration.
 *
 * Une étape dont max_ms est illimité peut être validée par l'écoulement du
 * temps lorsque min_ms est atteint. Cette règle permet notamment de
 * reconnaître un appui long sans attendre le relâchement.
 *
 * Les états de résultat sont les suivants :
 *
 *   - PS_ACTIVE       :  pattern en attente de son premier appui ; 
 *                        Un pattern dont error_config vaut true reste PS_INACTIVE et 
 *                        n'est jamais réactivé par pattern_matcher_reset().
 *   - PS_PENDING      :  pattern commencé et toujours possible ;
 *   - PS_INACTIVE     :  pattern impossible ou non correspondant ;
 *   - PS_JUST_MATCHED :  pattern reconnu pendant la mise à jour courante ;
 *   - PS_MATCHED      :  pattern reconnu lors d'une mise à jour précédente.
 *
 * error_config est indépendant de l'état d'exécution. Il est vrai uniquement
 * lorsqu'une définition de pattern est invalide. Dans ce cas, l'état du
 * pattern est toujours PS_INACTIVE.
 *
 *   Note : Lorsqu'une étape illimitée atteint sa durée minimale, elle est franchie
 *          automatiquement. Cette règle permet notamment de reconnaître un appui long
 *          sans attendre le relâchement.
 *      Exemple de pattern concerné :
 *          static const PatternStep_t long_first_click_steps[] = { 
 *              { 2000, 0 }, //reconnaître un appui long sans attendre le relâchement
 *              { 0, 300 } 
 *          };
 *      -> Résultat : Un appui de plus de 2s passera immédiatement l'étape, même si le relâchement n'a pas encore eu lieu.
 *                    L'étape suivante ne sera jamais validée et l'état de ce pattern passera à PS_INACTIVE.
 *                    Pour être validé, l'utilisateur devra relacher avant 2s
 *
 * Exemple : clic court, appui long et double-clic logique.
 *
 *
 *   static const PatternStep_t short_click_steps[] = {
 *       { 0, 500 }
 *   };
 *
 *   static const PatternStep_t long_press_steps[] = {
 *       { 2000, PATTERN_UNLIMITED_MS }
 *   };
 *
 *   static const PatternStep_t double_click_steps[] = {
 *       { 0, 300 },  // premier appui
 *       { 0, 300 },  // relâchement entre les deux clics
 *       { 0, 0 }     // second appui, durée non limitée
 *   };
 *
 *   static const MatcherPattern_t patterns[] = {
 *       { short_click_steps,  1, 0 },
 *       { long_press_steps,   1, 1 },
 *       { double_click_steps, 3, 2 }
 *   };
 *
 *   static const MatcherPatternList_t pattern_list = {
 *       patterns, 3
 *   };
 *
 *   MatcherPatternState_t states[3];
 *   MatcherPatternGlobal_t matcher;
 *
 *   // pattern_matcher_init() retourne false si les arguments sont invalides ou si au moins un pattern contient une configuration incohérente.
 *   if (!pattern_matcher_init(
 *           &matcher,
 *           &button,
 *           &pattern_list,
 *           states,
 *           false)) {
 *       // Configuration invalide ou arguments invalides.
 *   }
 *
 *   MatcherPatternResult_t result =
 *       pattern_matcher_update(&matcher, now_ms);
 *
 *   if (result.any_matched &&
 *       pattern_matcher_is_matched(&matcher, 2)) {
 *       // Double-clic reconnu.
 *   }
 *
 * pattern_matcher_update() appelle input_update() pour l'entrée associée.
 * L'appelant ne doit donc pas appeler input_update(), tous les updates doivent
 * passer par pattern_matcher_update().
 *
 */

#ifndef LIBPIC_INPUT_PATTERN_H
#define LIBPIC_INPUT_PATTERN_H

#define PATTERN_UNLIMITED_MS 0
#define PATTERN_IS_MATCH_MASK 0x80

#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

// // Durée maximale d'une étape du pattern
// typedef struct {
//     uint16_t max_ticks;   // Durée maximale de l'étape (en ticks)
// } PatternStep_t;

typedef enum {
    PS_ACTIVE = 0x00u,
    PS_PENDING = 0x01u,
    PS_INACTIVE = 0x02u,
    PS_JUST_MATCHED = 0x80u, //bit7 à 1
    PS_MATCHED = 0x81u,      //bit7 à 1
    //PS_ERROR = 0x7F //On conserve le bit7 pour indiquer les etats matched/just_matched
} enum_PatternState;

typedef struct {
    uint16_t min_ms;   // Durée minimale de l'étape (en ms)
    uint16_t max_ms;   // Durée maximale de l'étape (en ms)
} PatternStep_t;

// Un pattern = une séquence d'étapes
typedef struct {
    const PatternStep_t *steps;   // Tableau des étapes (en Flash)
    uint8_t length;               // Nombre d'étapes
    uint8_t id;                   // Identifiant pour l'utilisateur
} MatcherPattern_t;

// Liste de patterns
typedef struct {
    const MatcherPattern_t *patterns;    // Tableau de patterns (en Flash)
    uint8_t count;                       // Nombre de patterns
} MatcherPatternList_t;

// État interne d'un pattern (en RAM)
typedef struct {
    uint8_t step_index;         // Étape actuelle (0 = première)
    time_ms_t step_start_ms;    // Temps de début de l'étape actuelle
    enum_PatternState state;    // État actuel du pattern
    bool error_config;                 // true si le pattern est en erreur
} MatcherPatternState_t;

// Résultat retourné par pattern_matcher_update
typedef struct {
    bool any_matched;        // true si au moins un pattern a été validé
} MatcherPatternResult_t;

// Structure principale du matcher
typedef struct {
    const MatcherPatternList_t *list;   // Liste des patterns (définition)
    MatcherPatternState_t *states;      // États des patterns (fourni par l'utilisateur)
    input_t *input;                     // ← Le matcher connaît SON input
    bool paused;                        // true = ignore les ticks
    //time_ms_t last_update_time;         // Dernier temps de mise à jour
} MatcherPatternGlobal_t;

// Initialisation du matcher
bool pattern_matcher_init(MatcherPatternGlobal_t *matcher,
                        input_t *input,               
                        const MatcherPatternList_t *pattern_list,
                        MatcherPatternState_t *states,
                        bool paused);

// Mise à jour : analyse les patterns en fonction de l'état de l'input
// input : pointeur vers l'input à surveiller
// now_ms : temps actuel en millisecondes
// Retourne any_matched = true si au moins un pattern a été validé
MatcherPatternResult_t pattern_matcher_update(MatcherPatternGlobal_t *matcher, time_ms_t now_ms);

// Reset : réinitialise tous les états (tous les patterns redeviennent actifs)
void pattern_matcher_reset(MatcherPatternGlobal_t *matcher);

// Pause / Reprise
void pattern_matcher_pause(MatcherPatternGlobal_t *matcher);
void pattern_matcher_resume(MatcherPatternGlobal_t *matcher);

// Indique si tous les patterns sont invalidés
bool pattern_matcher_all_inactive(const MatcherPatternGlobal_t *matcher);

// Indique si un pattern spécifique (par ID) a été validé
bool pattern_matcher_is_matched(const MatcherPatternGlobal_t *matcher, uint8_t id);

#ifdef __cplusplus
}
#endif

#endif // LIBPIC_INPUT_PATTERN_H
