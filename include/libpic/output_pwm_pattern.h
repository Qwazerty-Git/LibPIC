/**
 * @file output_pwm_pattern.h
 * @brief Exécution de séquences de rapports cycliques sur une sortie PWM.
 *
 * Un pattern (OutputPwmPatternSequence_t) décrit une suite d'étapes
 * (OutputPwmPatternStep_t). Chaque étape impose un rapport cyclique à la sortie
 * pendant une durée donnée. Le rapport cyclique est un pourcentage : 0 coupe la
 * sortie, OUTPUT_PWM_DUTY_MAX l'active en permanence, les valeurs intermédiaires
 * la modulent. Une même séquence peut donc éteindre, allumer ou moduler.
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
 *   output_pwm_pattern_run(&pattern, &sequence, states, ARRAY_SIZE(states), ...);
 *
 * sizeof est un opérateur du langage : le rapport est une constante évaluée
 * par le compilateur, aucune division n'est exécutée par le PIC et aucune
 * routine de division n'est liée (mesuré sur PIC16F15214 avec XC8 4.00 :
 * environ 70 octets de Flash de moins qu'une division sur variables). Le motif
 * ne s'applique qu'à un vrai tableau, jamais à un pointeur.
 *
 * Le module pilote lui-même la sortie associée lors de
 * output_pwm_pattern_init(). Les changements de rapport cyclique de cette sortie
 * doivent donc passer exclusivement par les fonctions output_pwm_pattern_*() :
 * l'application ne doit pas appeler output_pwm_set_duty(), output_pwm_enable()
 * ou output_pwm_disable() tant qu'un pattern est en cours d'exécution.
 *
 * Un cycle correspond à un parcours complet de la séquence. cycle_target fixe
 * le nombre de cycles à réaliser ; la valeur PATTERN_UNLIMITED_CYCLES (0) désigne
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
 * compteur monotone que celui utilisé par output_pwm_pattern_update().
 *
 * L'intervalle entre deux appels doit rester inférieur à 65,5 s : time_ms_t est
 * codé sur 16 bits et les durées sont calculées modulo 65 536 ms. Une pause ou
 * un arrêt long n'est pas concerné, seul compte l'intervalle entre deux appels.
 *
 * La résolution temporelle dépend donc uniquement de la fréquence d'appel de
 * output_pwm_pattern_update() : c'est à l'application de choisir un tick adapté à
 * ce qu'elle pilote. Le module ne rattrape pas les étapes échues pendant une
 * absence d'appel et n'avance que d'une étape par appel. Une étape dure donc
 * toujours au moins duration_ms, et au plus duration_ms augmenté d'une période
 * d'appel. Ce choix garantit qu'aucun état de sortie n'est abrégé (une phase
 * n'est jamais plus courte que demandé), au prix d'une dérive cumulée si le
 * tick est irrégulier. Pour un pilotage synchronisé sur un dispositif
 * externe, préférer un module piloté par entrée.
 *
 * output_pwm_pattern_pause() et output_pwm_pattern_stop() imposent un rapport
 * cyclique à la sortie : soit une valeur de 0 à OUTPUT_PWM_DUTY_MAX, soit
 * OUTPUT_PWM_DUTY_KEEP pour laisser la sortie telle quelle.
 * En pause, les étapes et les cycles continuent d'être consommés
 * (les étapes échues sont marquées OUTPUT_STEP_SKIPPED, sauf dépassement de durée)
 * mais la sortie reste dans l'état imposé. La pause ne suspend donc
 * pas le contrat temporel : l'application doit continuer d'appeler
 * output_pwm_pattern_update() au moins une fois par durée d'étape.
 * Le temps n'étant pas figé, une fin de cycle peut survenir pendant la pause,
 * et le pattern peut se terminer sans que la sortie ait été modifiée.
 * À chaque redémarrage de cycle, les états des étapes sont réinitialisés
 * (OUTPUT_STEP_PENDING) : l'échec d'une étape reste visible jusqu'au
 * redémarrage du cycle suivant au plus tard. Si l'étape fautive est la dernière
 * d'un cycle qui recommence, elle est réinitialisée dans le même appel : seul
 * new_failed porte alors la trace de l'échec.
 *
 * Pour suspendre réellement un pattern, utiliser output_pwm_pattern_stop(),
 * qui fige le pattern et mémorise le temps déjà écoulé dans l'étape courante
 * afin que output_pwm_pattern_resume() reprenne exactement là où il avait été arrêté.
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
 * Exemple : montée puis extinction de trois cycles.
 *
 *   static const OutputPwmPatternStep_t breath_steps[] = {
 *       { 20,  100 },   // 20 % pendant 100 ms
 *       { 100, 300 }    // sortie pleine pendant 300 ms
 *   };
 *
 *   static const OutputPwmPatternSequence_t breath_sequence = {
 *       breath_steps, 2
 *   };
 *
 *   static OutputPatternStepState_t breath_states[2];
 *   static OutputPwmPattern_t breath_pattern;
 *
 *   output_pwm_pattern_init(&breath_pattern, &led);
 *   output_pwm_pattern_run(&breath_pattern, &breath_sequence, breath_states,
 *                      ARRAY_SIZE(breath_states), 3, false, 0,
 *                      now_ms);
 *
 *   // Puis, périodiquement :
 *   OutputPatternEvent_t event =
 *       output_pwm_pattern_update(&breath_pattern, now_ms);
 *
 *   if (event.pattern_state == PATTERN_EXEC_COMPLETED) {
 *       // Les trois cycles ont été réalisés.
 *   }
 */

#ifndef LIBPIC_OUTPUT_PWM_PATTERN_H
#define LIBPIC_OUTPUT_PWM_PATTERN_H

#include "output_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Décrit une étape d'un pattern PWM. */
typedef struct {
    uint8_t value;             /**< Rapport cyclique imposé, de 0 à OUTPUT_PWM_DUTY_MAX. */
    time_ms_t duration_ms;     /**< Durée de l'étape, en millisecondes. */
} OutputPwmPatternStep_t;

/** Séquence d'étapes formant un pattern PWM. */
typedef struct {
    const OutputPwmPatternStep_t *steps;   /**< Tableau des étapes (en Flash). */
    uint8_t length;                     /**< Nombre d'étapes. */
} OutputPwmPatternSequence_t;

/** Structure principale d'un pattern de sortie PWM. */
typedef struct {
    const OutputPwmPatternSequence_t *sequence;   /**< Séquence à réaliser (transmise par l'utilisateur). */
    OutputPatternStepState_t *states;          /**< États des étapes (en RAM, fournis par l'utilisateur). */
    output_pwm_t *output;                          /**< Sortie PWM pilotée par le pattern. */
    PatternExecutionState_t execution_state;   /**< État d'exécution courant. */
    uint8_t cycle_target;                     /**< Nombre total de cycles à réaliser, 0 = illimité. */
    uint8_t cycles_done;                      /**< Nombre de cycles réalisés (modulo 256). */
    bool abort_on_fail;                        /**< true pour interrompre le pattern au premier échec. */
    uint8_t state_on_fail;               /**< Rapport cyclique appliqué en cas d'échec, ou OUTPUT_PWM_DUTY_KEEP. */

    /* Champs internes, gérés par le module : ne pas modifier directement. */
    bool flag_any_failed;                      /**< Échec détecté depuis le dernier événement. */
    bool flag_any_skipped;                     /**< Étape passée depuis le dernier événement. */
    uint8_t current_step_id;                   /**< Index de l'étape en cours d'exécution. */
} OutputPwmPattern_t;

/**
 * Initialise un pattern et l'associe à une sortie.
 *
 * @param pattern Pattern à initialiser.
 * @param output  Sortie pilotée par le pattern (doit rester valide).
 * @return true si les arguments sont valides, false sinon.
 *
 * @note Le pattern est laissé à l'arrêt (PATTERN_EXEC_STOPPED) ; utiliser
 *       output_pwm_pattern_run() pour démarrer une séquence.
 */
bool output_pwm_pattern_init(OutputPwmPattern_t *pattern, output_pwm_t *output);

/**
 * Met à jour le pattern et la sortie associée. Doit être appelée
 * périodiquement.
 *
 * @note Le module pilote la sortie : l'application ne doit pas appeler
 *       output_pwm_set_duty(), output_pwm_enable() ou output_pwm_disable() sur
 *       cette sortie pendant l'exécution d'un pattern. Le rapport cyclique d'une
 *       étape est appliqué à son démarrage, et à chaque changement d'étape tant
 *       que le pattern est en PATTERN_EXEC_RUNNING ; la sortie n'est pas
 *       touchée pendant une pause.
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
OutputPatternEvent_t output_pwm_pattern_update(OutputPwmPattern_t *pattern, time_ms_t now_ms);

/**
 * Transmet la séquence à exécuter et démarre le pattern.
 *
 * @param pattern       Pattern à démarrer.
 * @param sequence      Séquence à réaliser (doit rester valide).
 * @param states        Tableau d'états en RAM, d'exactement sequence->length
 *                      éléments (un par étape).
 * @param states_count  Nombre d'éléments de states : doit valoir
 *                      sequence->length (passer ARRAY_SIZE(states)).
 * @param cycle_target  Nombre de cycles à réaliser, PATTERN_UNLIMITED_CYCLES pour
 *                      un nombre illimité.
 * @param abort_on_fail true pour interrompre le pattern au premier échec.
 * @param state_on_fail Rapport cyclique appliqué à la sortie en cas d'échec
 *                      (0 à OUTPUT_PWM_DUTY_MAX), ou OUTPUT_PWM_DUTY_KEEP.
 * @param now_ms        Instant de démarrage en millisecondes.
 * @return true si les arguments sont valides, false sinon (pointeur NULL,
 *         séquence vide, taille du tableau d'états incorrecte).
 *
 * @note L'état de la première étape est appliqué immédiatement à la sortie ;
 *       les cycles repartent de zéro (les états précédents sont effacés).
 *       Les rapports cycliques des étapes et state_on_fail ne sont pas validés
 *       ici : output_pwm_set_duty() refuse toute valeur supérieure à
 *       OUTPUT_PWM_DUTY_MAX et l'application doit surveiller new_failed.
 */
bool output_pwm_pattern_run(OutputPwmPattern_t *pattern,
                        const OutputPwmPatternSequence_t *sequence,
                        OutputPatternStepState_t *states,
                        uint8_t states_count,
                        uint8_t cycle_target,
                        bool abort_on_fail,
                        uint8_t state_on_fail,
                        time_ms_t now_ms);

/**
 * Réinitialise toutes les étapes de la séquence en cours et arrête le pattern.
 *
 * @param pattern Pattern à réinitialiser.
 *
 * @note Les étapes repassent à OUTPUT_STEP_PENDING et le compteur de cycles
 *       est remis à zéro. L'état d'exécution devient PATTERN_EXEC_STOPPED ;
 *       utiliser output_pwm_pattern_resume() pour relancer le pattern.
 */
void output_pwm_pattern_reset(OutputPwmPattern_t *pattern);

/**
 * Arrête le pattern et fige la sortie au rapport cyclique demandé.
 *
 * @param pattern      Pattern à arrêter.
 * @param output_state Rapport cyclique imposé à la sortie (0 à
 *                     OUTPUT_PWM_DUTY_MAX), ou OUTPUT_PWM_DUTY_KEEP pour la
 *                     laisser telle quelle.
 * @param now_ms       Instant courant en millisecondes.
 * @return Instantané de l'exécution au moment de l'arrêt.
 *
 * @note Le temps déjà écoulé dans l'étape courante est mémorisé afin que
 *       output_pwm_pattern_resume() reprenne l'étape là où elle a été figée.
 */
OutputPatternEvent_t output_pwm_pattern_stop(OutputPwmPattern_t *pattern, uint8_t output_state, time_ms_t now_ms);

/**
 * Met le pattern en pause et fige la sortie au rapport cyclique demandé.
 *
 * @param pattern      Pattern à mettre en pause.
 * @param output_state Rapport cyclique imposé à la sortie (0 à
 *                     OUTPUT_PWM_DUTY_MAX), ou OUTPUT_PWM_DUTY_KEEP pour la
 *                     laisser telle quelle.
 * @param now_ms       Instant courant en millisecondes.
 * @return Instantané de l'exécution au moment de la pause.
 *
 * @note Une mise à jour est effectuée avant la mise en pause afin de ne pas
 *       perdre une étape qui se terminerait à cet instant. La sortie reste
 *       ensuite dans output_state pendant toute la pause, alors que les étapes
 *       et les cycles continuent d'être consommés ; le contrat temporel
 *       d'output_pwm_pattern_update() reste donc valable. La pause n'a d'effet que
 *       sur un pattern en PATTERN_EXEC_RUNNING.
 */
OutputPatternEvent_t output_pwm_pattern_pause(OutputPwmPattern_t *pattern, uint8_t output_state, time_ms_t now_ms);

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
OutputPatternEvent_t output_pwm_pattern_resume(OutputPwmPattern_t *pattern, time_ms_t now_ms);

/**
 * Indique si le pattern est terminé.
 *
 * @param pattern Pattern à consulter.
 * @return true si le nombre de cycles demandé a été réalisé ; false si des
 *         cycles restent à faire ou si cycle_target vaut
 *         PATTERN_UNLIMITED_CYCLES.
 */
bool output_pwm_pattern_is_completed(const OutputPwmPattern_t *pattern);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_OUTPUT_PWM_PATTERN_H */
