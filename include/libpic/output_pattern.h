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
 *   - un buffer d'états (OutputPatternStateBuffer_t) en RAM, dont la longueur
 *     doit être identique à celle de la séquence.
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
 * output_pattern_pause() et output_pattern_stop() imposent un état à la
 * sortie (OUTPUT_ON, OUTPUT_OFF ou OUTPUT_KEEP). stop() mémorise le temps déjà
 * écoulé dans l'étape courante afin que output_pattern_resume() reprenne
 * exactement là où le pattern avait été figé.
 *
 * Si abort_on_fail vaut true, le premier échec d'une étape interrompt le
 * pattern et applique state_on_fail à la sortie.
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
 *   static OutputPatternStateBuffer_t blink_buffer = {
 *       blink_states, 2
 *   };
 *
 *   static OutputPattern_t blink_pattern;
 *
 *   output_pattern_init(&blink_pattern, &led);
 *   output_pattern_run(&blink_pattern, &blink_sequence, &blink_buffer,
 *                      3, false, OUTPUT_OFF, now_ms);
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
    OUTPUT_STEP_FAILED = 0x04u       /**< Échec. */
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

/** Buffer d'états fourni par l'application, de même longueur que la séquence. */
typedef struct {
    OutputPatternStepState_t *data;   /**< Tableau d'états des étapes (en RAM). */
    uint8_t length;                   /**< Nombre d'états du tableau. */
} OutputPatternStateBuffer_t;

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
    OutputPatternStateBuffer_t *state_buffer;  /**< États des étapes (fourni par l'utilisateur). */
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
 *       l'exécution d'un pattern.
 *
 * @param pattern Pattern à mettre à jour.
 * @param now_ms  Instant courant en millisecondes.
 * @return Instantané de l'exécution après la mise à jour.
 */
OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms);

/**
 * Transmet la séquence à exécuter et démarre le pattern.
 *
 * @param pattern      Pattern à démarrer.
 * @param sequence     Séquence à réaliser (doit rester valide).
 * @param state_buffer Buffer d'états en RAM, de même longueur que la séquence.
 * @param cycle_target Nombre de cycles à réaliser, PATTERN_UNLIMITED_MS pour
 *                     un nombre illimité.
 * @param abort_on_fail true pour interrompre le pattern au premier échec.
 * @param state_on_fail État appliqué à la sortie en cas d'échec.
 * @param now_ms       Instant de démarrage en millisecondes.
 * @return true si les arguments sont valides, false sinon (pointeur NULL,
 *         séquence vide ou longueurs incohérentes).
 */
bool output_pattern_run(OutputPattern_t *pattern, const OutputPatternSequence_t *sequence, OutputPatternStateBuffer_t *state_buffer, uint16_t cycle_target, bool abort_on_fail, TargetState_t state_on_fail, time_ms_t now_ms);

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
 *       perdre une étape qui se terminerait à cet instant.
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
 * Indique si toutes les étapes de la séquence ont été traitées.
 *
 * @param pattern Pattern à consulter.
 * @return true si toutes les étapes sont traitées ; des cycles peuvent
 *         toutefois rester à réaliser.
 */
bool output_pattern_all_completed(const OutputPattern_t *pattern);

/**
 * Indique si le dernier cycle de la séquence est terminé.
 *
 * @param pattern Pattern à consulter.
 * @return false si un cycle est encore en cours, true si le dernier cycle est
 *         terminé.
 */
bool output_pattern_loop_over(const OutputPattern_t *pattern);

/**
 * Indique si le pattern est terminé.
 *
 * @param pattern Pattern à consulter.
 * @return true si le nombre de cycles demandé a été réalisé ; false si des
 *         cycles restent à faire ou si cycle_target vaut
 *         PATTERN_UNLIMITED_MS.
 */
bool output_pattern_is_completed(const OutputPattern_t *pattern);

/**
 * Retourne un instantané de l'exécution pour une étape donnée.
 *
 * @param pattern Pattern à consulter.
 * @param step_id Index de l'étape concernée (0 = première).
 * @return Instantané de l'exécution du pattern.
 */
OutputPatternEvent_t output_pattern_state(const OutputPattern_t *pattern, uint8_t step_id);

#ifdef __cplusplus
}
#endif

#endif // LIBPIC_OUTPUT_PATTERN_H
