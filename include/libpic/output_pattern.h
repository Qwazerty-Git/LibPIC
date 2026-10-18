/**
 * @file output_pattern.h
 * @brief Exécution de séquences d'états sur une sortie numérique.
 *
 * Un pattern (OutputPatternSequence_t) décrit une suite d'étapes
 * (OutputPatternStep_t). Chaque étape impose un état logique à la sortie
 * pendant une durée donnée.
 *
 * L'application fournit :
 *
 *   - la séquence, généralement déclarée en Flash ;
 *   - un tableau d'états en RAM (OutputPatternStepState_t), d'exactement
 *     sequence->length éléments : un état par étape.
 *
 * La taille du tableau d'états peut être transmise sans risque de
 * désynchronisation en la calculant à partir du tableau lui-même :
 *
 *   #define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
 *
 *   output_pattern_run(&pattern, &sequence, states, ARRAY_SIZE(states), ...);
 *
 * sizeof est un opérateur du langage : le rapport est une constante évaluée
 * par le compilateur, aucune division n'est exécutée par le PIC et aucune
 * routine de division n'est liée (mesuré sur PIC16F15213 avec XC8 4.00 :
 * environ 70 octets de Flash de moins qu'une division sur variables). Le motif
 * ne s'applique qu'à un vrai tableau, jamais à un pointeur.
 *
 * Le module pilote lui-même la sortie associée lors de
 * output_pattern_init(). Les changements d'état de cette sortie doivent donc
 * passer exclusivement par les fonctions output_pattern_*() : l'application ne
 * doit pas appeler output_set(), output_on() ou output_off() tant qu'un
 * pattern est en cours d'exécution.
 *
 * Un cycle correspond à un parcours complet de la séquence. cycle_target fixe
 * le nombre de cycles à réaliser ; la valeur PATTERN_UNLIMITED_MS (0) désigne
 * un nombre illimité de cycles.
 *
 * Les états d'exécution sont décrits par PatternExecutionState_t :
 *
 *   - PATTERN_EXEC_STOPPED   : arrêté, aucun cycle en cours ;
 *   - PATTERN_EXEC_PAUSED    : en pause ; le temps continue de s'écouler et
 *                              les étapes échues sont marquées
 *                              OUTPUT_STEP_SKIPPED ;
 *   - PATTERN_EXEC_RUNNING   : en cours d'exécution ;
 *   - PATTERN_EXEC_COMPLETED : tous les cycles demandés ont été réalisés ;
 *   - PATTERN_EXEC_ABORDED   : interrompu à la suite d'un échec.
 *
 * Le temps de chaque étape est mesuré depuis son début avec un horodatage
 * absolu en millisecondes. Les valeurs now_ms doivent provenir du même
 * compteur monotone que celui utilisé par output_pattern_update().
 *
 * L'intervalle entre deux appels doit rester inférieur à 65,5 s : time_ms_t est
 * codé sur 16 bits et les durées sont calculées modulo 65 536 ms. Une pause ou
 * un arrêt long n'est pas concerné, seul compte l'intervalle entre deux appels.
 *
 * La résolution temporelle dépend donc uniquement de la fréquence d'appel de
 * output_pattern_update() : c'est à l'application de choisir un tick adapté à
 * ce qu'elle pilote. Le module ne rattrape pas les étapes échues pendant une
 * absence d'appel et n'avance que d'une étape par appel. Une étape dure donc
 * toujours au moins duration_ms, et au plus duration_ms augmenté d'une période
 * d'appel. Ce choix garantit qu'aucun état de sortie n'est abrégé (une phase
 * n'est jamais plus courte que demandé), au prix d'une dérive cumulée si le
 * tick est irrégulier. Pour un pilotage synchronisé sur un dispositif
 * externe, préférer un module piloté par entrée.
 *
 * output_pattern_pause() et output_pattern_stop() imposent un état à la
 * sortie (OUTPUT_ON, OUTPUT_OFF ou OUTPUT_KEEP).
 * En pause, les étapes et les cycles continuent d'être consommés
 * (les étapes échues sont marquées OUTPUT_STEP_SKIPPED, sauf dépassement de durée)
 * mais la sortie reste dans l'état imposé. La pause ne suspend donc
 * pas le contrat temporel : l'application doit continuer d'appeler
 * output_pattern_update() au moins une fois par durée d'étape.
 * Le temps n'étant pas figé, une fin de cycle peut survenir pendant la pause,
 * et le pattern peut se terminer sans que la sortie ait été modifiée.
 * À chaque redémarrage de cycle, les états des étapes sont réinitialisés
 * (OUTPUT_STEP_PENDING) : l'échec d'une étape reste visible jusqu'au
 * redémarrage du cycle suivant au plus tard. Si l'étape fautive est la dernière
 * d'un cycle qui recommence, elle est réinitialisée dans le même appel : seul
 * new_failed porte alors la trace de l'échec.
 *
 * Pour suspendre réellement un pattern, utiliser output_pattern_stop(),
 * qui fige le pattern et mémorise le temps déjà écoulé dans l'étape courante
 * afin que output_pattern_resume() reprenne exactement là où il avait été arrêté.
 *
 * À la fin du dernier cycle, la sortie conserve l'état de la dernière étape,
 * ou l'état imposé lors de la dernière pause ou du dernier arrêt.
 *
 * Une étape est marquée OUTPUT_STEP_FAILED lorsque son dépassement de durée
 * (temps écoulé depuis son départ moins duration_ms) atteint au moins la durée
 * de l'étape suivante : le tick d'appel est alors trop long pour que la phase
 * suivante puisse être honorée, une phase entière a été avalée entre deux
 * appels. L'étape qui termine le dernier cycle n'a pas d'étape suivante : un
 * dépassement n'y est donc pas un échec. Ce critère s'applique en marche comme
 * en pause ; en pause, une étape échue sans dépassement est marquée
 * OUTPUT_STEP_SKIPPED.
 *
 * Si abort_on_fail vaut true, le premier échec d'une étape interrompt le
 * pattern (PATTERN_EXEC_ABORDED) et applique state_on_fail à la sortie. Sinon
 * le pattern poursuit sa séquence : l'application surveille new_failed, ou
 * inspecte les états de son tableau, pour réagir elle-même.
 *
 * Les événements retournés (OutputPatternEvent_t) sont des instantanés :
 * new_failed et new_skipped signalent les échecs et les étapes passées depuis
 * l'appel précédent, puis sont remis à zéro.
 *
 * Exemple : clignotement de trois cycles.
 *
 *   static const OutputPatternStep_t blink_steps[] = {
 *       { true,  100 },   // sortie active pendant 100 ms
 *       { false, 300 }    // sortie inactive pendant 300 ms
 *   };
 *
 *   static const OutputPatternSequence_t blink_sequence = {
 *       blink_steps, 2
 *   };
 *
 *   static OutputPatternStepState_t blink_states[2];
 *   static OutputPattern_t blink_pattern;
 *
 *   output_pattern_init(&blink_pattern, &led);
 *   output_pattern_run(&blink_pattern, &blink_sequence, blink_states,
 *                      ARRAY_SIZE(blink_states), 3, false, OUTPUT_OFF,
 *                      now_ms);
 *
 *   // Puis, périodiquement :
 *   OutputPatternEvent_t event =
 *       output_pattern_update(&blink_pattern, now_ms);
 *
 *   if (event.pattern_state == PATTERN_EXEC_COMPLETED) {
 *       // Les trois cycles de clignotement ont été réalisés.
 *   }
 */

#ifndef LIBPIC_OUTPUT_PATTERN_H
#define LIBPIC_OUTPUT_PATTERN_H

/** Nombre de cycles illimité (valeur sentinelle de cycle_target). */
#define PATTERN_UNLIMITED_MS 0

#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

/** États possibles d'une étape durant l'exécution d'un pattern. */
typedef enum {
    OUTPUT_STEP_PENDING = 0x00u,     /**< Pas encore commencée. */
    OUTPUT_STEP_RUNNING = 0x01u,     /**< En cours. */
    OUTPUT_STEP_COMPLETED = 0x02u,   /**< Réalisée normalement. */
    OUTPUT_STEP_SKIPPED = 0x03u,     /**< Passée sans être exécutée. */
    OUTPUT_STEP_FAILED = 0x04u       /**< Échec : dépassement de durée trop important (voir @file). */
} OutputPatternStepStateEnum_t;

/** Décrit une étape d'un pattern. */
typedef struct {
    bool active;             /**< État de la sortie durant l'étape (true = actif). */
    uint16_t duration_ms;    /**< Durée de l'étape, en millisecondes. */
} OutputPatternStep_t;

/** Séquence d'étapes formant un pattern. */
typedef struct {
    const OutputPatternStep_t *steps;   /**< Tableau des étapes (en Flash). */
    uint8_t length;                     /**< Nombre d'étapes. */
} OutputPatternSequence_t;

/** État interne d'une étape (en RAM). */
typedef struct {
    time_ms_t step_start_ms;                   /**< Temps de début de l'étape actuelle. */
    OutputPatternStepStateEnum_t state;        /**< État actuel de l'étape. */
} OutputPatternStepState_t;

/** Résultat retourné par output_pattern_update() et par les fonctions d'arrêt. */
typedef struct {
    PatternExecutionState_t pattern_state;   /**< État d'exécution du pattern. */
    uint8_t current_step;                    /**< Index de l'étape en cours (0 = première). */
    uint16_t cycles_done;                    /**< Nombre de cycles réalisés. */
    bool new_failed;                         /**< true si au moins une étape a échoué depuis l'appel précédent. */
    bool new_skipped;                        /**< true si au moins une étape a été passée depuis l'appel précédent. */
} OutputPatternEvent_t;

/** Structure principale d'un pattern de sortie. */
typedef struct {
    const OutputPatternSequence_t *sequence;   /**< Séquence à réaliser (transmise par l'utilisateur). */
    OutputPatternStepState_t *states;          /**< États des étapes (en RAM, fournis par l'utilisateur). */
    output_t *output;                          /**< Sortie pilotée par le pattern. */
    PatternExecutionState_t execution_state;   /**< État d'exécution courant. */
    uint16_t cycle_target;                     /**< Nombre total de cycles à réaliser, 0 = illimité. */
    uint16_t cycles_done;                      /**< Nombre de cycles réalisés. */
    bool abort_on_fail;                        /**< true pour interrompre le pattern au premier échec. */
    TargetState_t state_on_fail;               /**< État appliqué à la sortie en cas d'échec. */

    /* Champs internes, gérés par le module : ne pas modifier directement. */
    bool flag_any_failed;                      /**< Échec détecté depuis le dernier événement. */
    bool flag_any_skipped;                     /**< Étape passée depuis le dernier événement. */
    uint8_t current_step_id;                   /**< Index de l'étape en cours d'exécution. */
} OutputPattern_t;

/**
 * Initialise un pattern et l'associe à une sortie.
 *
 * @param pattern Pattern à initialiser.
 * @param output  Sortie pilotée par le pattern (doit rester valide).
 * @return true si les arguments sont valides, false sinon.
 *
 * @note Le pattern est laissé à l'arrêt (PATTERN_EXEC_STOPPED) ; utiliser
 *       output_pattern_run() pour démarrer une séquence.
 */
bool output_pattern_init(OutputPattern_t *pattern, output_t *output);

/**
 * Met à jour le pattern et la sortie associée. Doit être appelée
 * périodiquement.
 *
 * @note Le module pilote la sortie : l'application ne doit pas appeler
 *       output_set(), output_on() ou output_off() sur cette sortie pendant
 *       l'exécution d'un pattern. L'état de l'étape courante est réappliqué à
 *       chaque appel tant que le pattern est en PATTERN_EXEC_RUNNING ; la
 *       sortie n'est pas touchée pendant une pause.
 *
 * @note Une étape validée avec un dépassement d'au moins la durée de l'étape
 *       suivante est marquée OUTPUT_STEP_FAILED et l'événement porte
 *       new_failed. Avec abort_on_fail, le pattern passe en
 *       PATTERN_EXEC_ABORDED et la sortie reçoit state_on_fail. Le contrôle
 *       s'applique aussi en pause : seule la sortie y est figée, pas le temps.
 *
 * @param pattern Pattern à mettre à jour.
 * @param now_ms  Instant courant en millisecondes.
 * @return Instantané de l'exécution après la mise à jour.
 */
OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms);

/**
 * Transmet la séquence à exécuter et démarre le pattern.
 *
 * @param pattern       Pattern à démarrer.
 * @param sequence      Séquence à réaliser (doit rester valide).
 * @param states        Tableau d'états en RAM, d'exactement sequence->length
 *                      éléments (un par étape).
 * @param states_count  Nombre d'éléments de states : doit valoir
 *                      sequence->length (passer ARRAY_SIZE(states)).
 * @param cycle_target  Nombre de cycles à réaliser, PATTERN_UNLIMITED_MS pour
 *                      un nombre illimité.
 * @param abort_on_fail true pour interrompre le pattern au premier échec.
 * @param state_on_fail État appliqué à la sortie en cas d'échec.
 * @param now_ms        Instant de démarrage en millisecondes.
 * @return true si les arguments sont valides, false sinon (pointeur NULL,
 *         séquence vide, taille du tableau d'états incorrecte).
 *
 * @note L'état de la première étape est appliqué immédiatement à la sortie ;
 *       les cycles repartent de zéro (les états précédents sont effacés).
 */
bool output_pattern_run(OutputPattern_t *pattern,
                        const OutputPatternSequence_t *sequence,
                        OutputPatternStepState_t *states,
                        uint8_t states_count,
                        uint16_t cycle_target,
                        bool abort_on_fail,
                        TargetState_t state_on_fail,
                        time_ms_t now_ms);

/**
 * Réinitialise toutes les étapes de la séquence en cours et arrête le pattern.
 *
 * @param pattern Pattern à réinitialiser.
 *
 * @note Les étapes repassent à OUTPUT_STEP_PENDING et le compteur de cycles
 *       est remis à zéro. L'état d'exécution devient PATTERN_EXEC_STOPPED ;
 *       utiliser output_pattern_resume() pour relancer le pattern.
 */
void output_pattern_reset(OutputPattern_t *pattern);

/**
 * Arrête le pattern et fige la sortie dans l'état demandé.
 *
 * @param pattern      Pattern à arrêter.
 * @param output_state État imposé à la sortie (OUTPUT_ON, OUTPUT_OFF ou
 *                     OUTPUT_KEEP pour la conserver).
 * @param now_ms       Instant courant en millisecondes.
 * @return Instantané de l'exécution au moment de l'arrêt.
 *
 * @note Le temps déjà écoulé dans l'étape courante est mémorisé afin que
 *       output_pattern_resume() reprenne l'étape là où elle a été figée.
 */
OutputPatternEvent_t output_pattern_stop(OutputPattern_t *pattern, TargetState_t output_state, time_ms_t now_ms);

/**
 * Met le pattern en pause et fige la sortie dans l'état demandé.
 *
 * @param pattern      Pattern à mettre en pause.
 * @param output_state État imposé à la sortie (OUTPUT_ON, OUTPUT_OFF ou
 *                     OUTPUT_KEEP pour la conserver).
 * @param now_ms       Instant courant en millisecondes.
 * @return Instantané de l'exécution au moment de la pause.
 *
 * @note Une mise à jour est effectuée avant la mise en pause afin de ne pas
 *       perdre une étape qui se terminerait à cet instant. La sortie reste
 *       ensuite dans output_state pendant toute la pause, alors que les étapes
 *       et les cycles continuent d'être consommés ; le contrat temporel
 *       d'output_pattern_update() reste donc valable. La pause n'a d'effet que
 *       sur un pattern en PATTERN_EXEC_RUNNING.
 */
OutputPatternEvent_t output_pattern_pause(OutputPattern_t *pattern, TargetState_t output_state, time_ms_t now_ms);

/**
 * Reprend le pattern après un arrêt ou une pause.
 *
 * @param pattern Pattern à reprendre.
 * @param now_ms  Instant courant en millisecondes.
 * @return Instantané de l'exécution après la reprise.
 *
 * @note Un pattern déjà complet reste en PATTERN_EXEC_COMPLETED. Après un
 *       arrêt, le temps écoulé de l'étape courante est restauré.
 */
OutputPatternEvent_t output_pattern_resume(OutputPattern_t *pattern, time_ms_t now_ms);

/**
 * Indique si le pattern est terminé.
 *
 * @param pattern Pattern à consulter.
 * @return true si le nombre de cycles demandé a été réalisé ; false si des
 *         cycles restent à faire ou si cycle_target vaut
 *         PATTERN_UNLIMITED_MS.
 */
bool output_pattern_is_completed(const OutputPattern_t *pattern);

#ifdef __cplusplus
}
#endif

#endif // LIBPIC_OUTPUT_PATTERN_H
