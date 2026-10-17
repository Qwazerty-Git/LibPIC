/**
 * @file output_pattern.h
 * @brief Reconnaissance de séquences d'états sur une entrée numérique.
 *
 
 * output_pattern_update() appelle output_update() pour la sortie associée.
 * L'appelant ne doit donc pas appeler output_update(), tous les updates doivent
 * passer par outputpattern_update().


	OutputPatternStepState_t *states;
  Mémoire de travail fournie par l'application.
 Le module initialise et met à jour les états.
 L'application ne doit pas modifier ce tableau pendant l'exécution.

 *
 */

#ifndef LIBPIC_OUTPUT_PATTERN_H
#define LIBPIC_OUTPUT_PATTERN_H

#define PATTERN_UNLIMITED_MS 0
//#define PATTERN_IS_MATCH_MASK 0x80

#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OUTPUT_STEP_PENDING = 0x00u,  							// Pas encore commencée
    OUTPUT_STEP_RUNNING = 0x01u,  							// En cours
    OUTPUT_STEP_COMPLETED = 0x02u,							// Réalisée normalement
    OUTPUT_STEP_SKIPPED = 0x03u,  							// Passée sans être exécutée
    OUTPUT_STEP_FAILED = 0x04u    							// Échec
} OutputPatternStepStateEnum_t;

typedef struct {
    bool active;                        				// Etat de la sortie durant le step
    uint16_t duration_ms;               				// Durée du step (en ms)
} OutputPatternStep_t;

// Un pattern = une séquence d'étapes
typedef struct {
    const OutputPatternStep_t *steps;   				// Tableau des étapes (en Flash)
    uint8_t length;                     				// Nombre d'étapes
} OutputPatternSequence_t;

// État interne d'un step (en RAM)
typedef struct {
    time_ms_t step_start_ms;            				// Temps de début de l'étape actuelle
    OutputPatternStepStateEnum_t state;      		// État actuel du pattern
    //bool error_config;                  			// true si le pattern est en erreur
} OutputPatternStepState_t;

typedef struct {
    OutputPatternStepState_t *data;
    uint8_t length;
} OutputPatternStateBuffer_t;

// Résultat retourné par output_pattern_update
typedef struct {
		PatternExecutionState_t pattern_state;
		uint8_t current_step;												//0 pour le 1er step
		uint16_t cycles_done;
    bool new_failed;           									// true si au moins un step a subit un echec
		bool new_skipped;
} OutputPatternEvent_t;

// Structure principale
typedef struct {
		const OutputPatternSequence_t *sequence;    // Séquence à réalisé (transmise par l'utilisateur)
		OutputPatternStateBuffer_t *state_buffer;   // États des steps (fourni par l'utilisateur)

		output_t *output;                   				// ← Le pattern connaît SON output
		PatternExecutionState_t execution_state;                        // true = ignore les ticks et met à jour le temps de départ

		uint16_t cycle_target;											// Nbr total de cycle à réaliser. Infini si 0
		uint16_t cycles_done;												// Nbr de cycle effectué depuis le dernier appel à run

		bool abort_on_fail;
		TargetState_t state_on_fail;

		/* États internes utilisés dans l'implémentation -> Pas de modification direct */

		bool flag_any_failed; // Permet de remettre le flag à 0 et de détecter les nouveaux step Failed dans result
		bool flag_any_skipped;
		uint8_t current_step_id;		
} OutputPattern_t;

// Initialisation
bool output_pattern_init(OutputPattern_t *pattern, output_t *output); 

// output_pattern : pointeur vers l'output_pattern à utiliser
// now_ms : temps actuel en millisecondes
OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms);

// On transmet la séquence à exécuter
bool output_pattern_run(OutputPattern_t *pattern, const OutputPatternSequence_t *sequence, OutputPatternStateBuffer_t *state_buffer, uint16_t cycle_target, bool abort_on_fail, TargetState_t state_on_fail, time_ms_t now_ms);//tableau de même dimenssion que sequence (à vérifier)
//si loop -> reset auto
//sinon mise en pause


// Reset : réinitialise tous les step de la séquence en cours (steps en pending) + mise en pause
void output_pattern_reset(OutputPattern_t *pattern);

// Pause / Reprise

OutputPatternEvent_t output_pattern_stop(OutputPattern_t *pattern,TargetState_t output_state, time_ms_t now_ms);
OutputPatternEvent_t output_pattern_pause(OutputPattern_t *pattern,TargetState_t output_state, time_ms_t now_ms);
OutputPatternEvent_t output_pattern_resume(OutputPattern_t *pattern, time_ms_t now_ms);


// Indique si tous les steps sont traités (la séquence est complète, il reste peut-être des loop à réaliser)
bool output_pattern_all_completed(const OutputPattern_t *pattern);

// Indique si le dernier pattern a été traité
bool output_pattern_loop_over(const OutputPattern_t *pattern); // retourne false si loop en cours, true si la dernière loop est terminée

// Indique si le pattern est terminé
bool output_pattern_is_completed(const OutputPattern_t *pattern);

// Indique l'état d'un pattern spécifique
OutputPatternEvent_t output_pattern_state(const OutputPattern_t *pattern, uint8_t step_id);

#ifdef __cplusplus
}
#endif

#endif // LIBPIC_OUTPUT_PATTERN_H
