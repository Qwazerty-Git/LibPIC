/**
 * @file input_pattern.h
 * @brief Reconnaissance de séquences d'états sur une entrée numérique.
 *
 * Le module observe une entrée input_t et reconnaît des séquences d'appuis et
 * de relâchements après validation par l'anti-rebond.
 *
 * Plusieurs séquences peuvent être évaluées en parallèle : elles sont
 * regroupées dans un MatcherInputPatternList_t et chacune possède un
 * identifiant (id) propre à l'application.
 *
 * Une séquence est décrite par un tableau de InputPatternStep_t. Chaque étape
 * correspond à un état logique de l'entrée :
 *
 *   - étape paire  (0, 2, 4, ...) : entrée active, appui ;
 *   - étape impaire (1, 3, 5, ...) : entrée inactive, relâchement.
 *
 * Les événements sont logiques et indépendants du niveau électrique de la
 * broche. Un bouton actif-bas produit donc normalement un rising_edge lors de
 * l'appui physique, car l'état logique passe de inactif à actif.
 *
 * Une étape d'appui est démarrée par un rising_edge et terminée par un
 * falling_edge. Une étape de relâchement est démarrée par un falling_edge et
 * terminée par un rising_edge.
 *
 * Le premier rising_edge démarre la première étape. Il ne termine pas cette
 * étape et ne déclenche donc pas encore le contrôle de min_ms.
 *
 * Pour chaque étape :
 *
 *   - min_ms est la durée minimale exigée ;
 *   - max_ms est la durée maximale autorisée ;
 *   - max_ms == PATTERN_UNLIMITED_MS signifie qu'il n'y a pas de durée
 *     maximale.
 *
 * Une valeur max_ms non nulle et inférieure à min_ms constitue une erreur de
 * configuration.
 *
 * Le temps de chaque étape est mesuré depuis son début avec un horodatage
 * absolu en millisecondes. Les valeurs now_ms doivent provenir du même
 * compteur monotone que celui utilisé par input_update().
 *
 * Un événement inattendu ou non fiable invalide le pattern courant, mais ne
 * constitue pas une erreur de configuration.
 *
 * Une étape dont max_ms vaut PATTERN_UNLIMITED_MS ne possède aucune durée
 * maximale. Elle reste active jusqu'à l'événement attendu, même lorsque
 * min_ms est atteint.
 *
 * Exception : si cette étape est la dernière du pattern et que min_ms est
 * strictement supérieur à zéro, le pattern peut être reconnu dès que min_ms
 * est atteint, sans attendre le changement d'état suivant. Cette règle permet
 * de reconnaître un appui long.
 *
 * Pour une étape illimitée non finale, min_ms ne provoque pas le passage
 * automatique à l'étape suivante. L'étape suivante ne commence qu'après
 * l'événement attendu.
 *
 * Les états de résultat sont les suivants :
 *
 *   - IPS_ACTIVE       : pattern en attente de son premier appui ;
 *   - IPS_PENDING      : pattern commencé et toujours possible ;
 *   - IPS_INACTIVE     : pattern impossible ou non correspondant ;
 *   - IPS_JUST_MATCHED : pattern reconnu pendant la mise à jour courante ;
 *   - IPS_MATCHED      : pattern reconnu lors d'une mise à jour précédente.
 *
 * error_config est indépendant de l'état d'exécution. Il est vrai uniquement
 * lorsqu'une définition de pattern est invalide. Dans ce cas, l'état du
 * pattern est toujours IPS_INACTIVE.
 *
 * Un pattern dont error_config vaut true reste donc IPS_INACTIVE et n'est
 * jamais réactivé par input_pattern_matcher_reset().
 *
 * Exemple : appui long.
 *
 *   static const InputPatternStep_t long_press_steps[] = {
 *       { 2000, PATTERN_UNLIMITED_MS }
 *   };
 *
 * Le pattern est reconnu au premier update observé à partir de 2000 ms
 * d'appui. Le relâchement n'est pas nécessaire pour produire la
 * reconnaissance.
 *
 * Exemple : appui long suivi d'un relâchement.
 *
 *   static const InputPatternStep_t long_then_release_steps[] = {
 *       { 2000, PATTERN_UNLIMITED_MS },
 *       { 0, 300 }
 *   };
 *
 * L'appui doit durer au moins 2000 ms. Le falling_edge termine alors l'étape
 * d'appui et démarre l'étape de relâchement. Le relâchement doit ensuite être
 * observé dans un délai maximal de 300 ms, selon la définition de l'étape
 * suivante.
 *
 * Exemple : clic court, appui long et double-clic logique.
 *
 *   static const InputPatternStep_t short_click_steps[] = {
 *       { 0, 500 }
 *   };
 *
 *   static const InputPatternStep_t long_press_steps[] = {
 *       { 2000, PATTERN_UNLIMITED_MS }
 *   };
 *
 *   static const InputPatternStep_t double_click_steps[] = {
 *       { 0, 300 }, // appui 1, terminé au falling_edge
 *       { 0, 300 }, // relâchement, terminé au rising_edge
 *       { 0, 0 }    // appui 2, validé au rising_edge
 *   };
 *
 *   static const MatcherInputPatternSequence_t patterns[] = {
 *       { short_click_steps,  1, 0 },
 *       { long_press_steps,   1, 1 },
 *       { double_click_steps, 3, 2 }
 *   };
 *
 *   static const MatcherInputPatternList_t pattern_list = {
 *       patterns, 3
 *   };
 *
 *   MatcherInputPatternState_t states[3];
 *   MatcherInputPattern_t matcher;
 *
 *   // input_pattern_matcher_init() retourne false si les arguments sont
 *   // invalides ou si au moins un pattern contient une configuration
 *   // incohérente.
 *   if (!input_pattern_matcher_init(
 *           &matcher,
 *           &button,
 *           &pattern_list,
 *           states,
 *           false)) {
 *       // Configuration invalide ou arguments invalides.
 *   }
 *
 *   MatcherInputPatternResult_t result =
 *       input_pattern_matcher_update(&matcher, now_ms);
 *
 *   if (result.any_matched &&
 *       input_pattern_matcher_is_matched(&matcher, 2)) {
 *       // Double-clic reconnu.
 *   }
 *
 * input_pattern_matcher_update() appelle input_update() pour l'entrée
 * associée. L'appelant ne doit donc pas appeler input_update() : toutes les
 * mises à jour de l'entrée doivent passer par
 * input_pattern_matcher_update().
 */

#ifndef LIBPIC_INPUT_PATTERN_H
#define LIBPIC_INPUT_PATTERN_H

/** Durée maximale absente : l'étape attend l'événement sans limite de temps. */
#define PATTERN_UNLIMITED_MS 0

/** Bit de poids fort d'un état de pattern, positionné lorsque le pattern est reconnu. */
#define PATTERN_IS_MATCH_MASK 0x80

#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

/** États possibles d'un pattern durant la reconnaissance. */
typedef enum {
    IPS_ACTIVE = 0x00u,          /**< En attente du premier appui. */
    IPS_PENDING = 0x01u,         /**< Pattern commencé et toujours possible. */
    IPS_INACTIVE = 0x02u,        /**< Pattern impossible ou non correspondant. */
    IPS_JUST_MATCHED = 0x80u,    /**< Reconnu pendant la mise à jour courante. */
    IPS_MATCHED = 0x81u          /**< Reconnu lors d'une mise à jour précédente. */
} enum_InputPatternState;

/** Décrit une étape d'un pattern. */
typedef struct {
    uint16_t min_ms;   /**< Durée minimale de l'étape, en millisecondes. */
    uint16_t max_ms;   /**< Durée maximale, ou PATTERN_UNLIMITED_MS si aucune. */
} InputPatternStep_t;

/** Séquence d'étapes formant un pattern, avec son identifiant applicatif. */
typedef struct {
    const InputPatternStep_t *steps;   /**< Tableau des étapes (en Flash). */
    uint8_t length;                    /**< Nombre d'étapes. */
    uint8_t id;                        /**< Identifiant pour l'utilisateur. */
} MatcherInputPatternSequence_t;

/** Liste des patterns surveillés par un matcher. */
typedef struct {
    const MatcherInputPatternSequence_t *patterns;   /**< Tableau de patterns (en Flash). */
    uint8_t count;                                   /**< Nombre de patterns. */
} MatcherInputPatternList_t;

/** État interne d'un pattern (en RAM). */
typedef struct {
    uint8_t step_index;                /**< Étape actuelle (0 = première). */
    time_ms_t step_start_ms;           /**< Temps de début de l'étape actuelle. */
    enum_InputPatternState state;      /**< État actuel du pattern. */
    bool error_config;                 /**< true si le pattern est en erreur de configuration. */
} MatcherInputPatternState_t;

/** Résultat retourné par input_pattern_matcher_update(). */
typedef struct {
    bool any_matched;     /**< true si au moins un pattern a été validé. */
    bool all_inactive;    /**< true si tous les patterns sont inactifs. */
} MatcherInputPatternResult_t;

/** Structure principale du matcher. */
typedef struct {
    const MatcherInputPatternList_t *list;   /**< Liste des patterns (définition). */
    MatcherInputPatternState_t *states;      /**< États des patterns (fourni par l'utilisateur). */
    input_t *input;                          /**< Entrée surveillée par le matcher. */
    bool paused;                             /**< true = ignore les mises à jour. */
} MatcherInputPattern_t;

/**
 * Initialise un matcher et valide la configuration de chaque pattern.
 *
 * @param matcher      Matcher à initialiser.
 * @param input        Entrée à surveiller.
 * @param pattern_list Liste des patterns à reconnaître (doit rester valide).
 * @param states       Tableau d'états en RAM, d'au moins pattern_list->count
 *                     éléments.
 * @param paused       true pour démarrer en pause.
 * @return true si la configuration est valide, false si un argument est NULL
 *         ou si au moins un pattern contient une erreur de configuration.
 */
bool input_pattern_matcher_init(MatcherInputPattern_t *matcher,
                        input_t *input,
                        const MatcherInputPatternList_t *pattern_list,
                        MatcherInputPatternState_t *states,
                        bool paused);

/**
 * Met à jour la reconnaissance des patterns. Doit être appelée périodiquement.
 *
 * @note Appelle input_update() pour l'entrée associée. L'application ne doit
 *       donc pas appeler input_update() elle-même.
 *
 * @param matcher Matcher à mettre à jour.
 * @param now_ms  Instant courant en millisecondes.
 * @return Résultat de la mise à jour ; any_matched vaut true si au moins un
 *         pattern vient d'être reconnu.
 */
MatcherInputPatternResult_t input_pattern_matcher_update(MatcherInputPattern_t *matcher, time_ms_t now_ms);

/**
 * Réinitialise tous les états : chaque pattern valide redevient IPS_ACTIVE.
 *
 * @note Les erreurs de configuration sont conservées : un pattern dont
 *       error_config vaut true reste IPS_INACTIVE.
 *
 * @param matcher Matcher à réinitialiser.
 */
void input_pattern_matcher_reset(MatcherInputPattern_t *matcher);

/**
 * Met le matcher en pause.
 *
 * @param matcher Matcher à mettre en pause.
 */
void input_pattern_matcher_pause(MatcherInputPattern_t *matcher);

/**
 * Reprend le matcher après une pause.
 *
 * @param matcher Matcher à reprendre.
 */
void input_pattern_matcher_resume(MatcherInputPattern_t *matcher);

/**
 * Indique si tous les patterns sont invalidés.
 *
 * @param matcher Matcher à consulter.
 * @return true si aucun pattern n'est encore possible (tous IPS_INACTIVE).
 */
bool input_pattern_matcher_all_inactive(const MatcherInputPattern_t *matcher);

/**
 * Indique si un pattern donné a été reconnu.
 *
 * @param matcher Matcher à consulter.
 * @param id      Identifiant du pattern (champ id de sa définition).
 * @return true si l'état du pattern vaut IPS_JUST_MATCHED ou IPS_MATCHED,
 *         false si le pattern est inconnu ou non reconnu.
 */
bool input_pattern_matcher_is_matched(const MatcherInputPattern_t *matcher, uint8_t id);

#ifdef __cplusplus
}
#endif

#endif // LIBPIC_INPUT_PATTERN_H
