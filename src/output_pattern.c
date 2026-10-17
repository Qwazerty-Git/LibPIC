/**
 * @file output_pattern.c
 * @brief Gestion de pattern sur une sortie.
 */

#import "libpic/output_pattern.h"

static OutputPatternEvent_t instant_event(OutputPattern_t *pattern,TargetState_t output_state);

// Initialisation
bool output_pattern_init(OutputPattern_t *pattern, output_t *output)
{
	if (pattern == NULL || output ==NULL) return false;

	pattern->output = output;
	output_pattern_reset(pattern);
	return true;
} 

// output_pattern : pointeur vers l'output_pattern à utiliser
// now_ms : temps actuel en millisecondes
OutputPatternEvent_t output_pattern_update(OutputPattern_t *pattern, time_ms_t now_ms)
{
	OutputPatternEvent_t event = instant_event(pattern,OUTPUT_KEEP);

	//A voir la modification à apporter à Event pour signaler un problème si la ligne suivant est validée -> probablement Abord si running à l'origine
	
	if (pattern == NULL || pattern->sequence == NULL || pattern->state_buffer == NULL || pattern->current_step_id < pattern->state_buffer->length) return event;

	// On regarde l'état d'execution du pattern
	if (pattern->execution_state == PATTERN_EXEC_ABORDED || 
		pattern->execution_state == PATTERN_EXEC_COMPLETED ||
		pattern->execution_state == PATTERN_EXEC_STOPPED) return event;

	// On pointe le step/state en cours
	const OutputPatternStep_t *current_step = &pattern->sequence->steps[pattern->current_step_id];
	OutputPatternStepState_t *current_state = &pattern->state_buffer->data[pattern->current_step_id];

	// // On regarde si le pattern doit débuter
	// if (pattern->execution_state == PATTERN_EXEC_STOPPED && current_state.state == OUTPUT_STEP_PENDING && pattern->cycles_done == 0)
	// {
	// 	// Le pattern subit son 1er update
	// 	current_state.step_start_ms = now_ms;
	// 	current_state.state = OUTPUT_STEP_RUNNING;
	// 	pattern->execution_state = PATTERN_EXEC_RUNNING;
	// }
	
	// On regarde si le step est validé
    if ((time_ms_t)(now_ms - current_state->step_start_ms) >= current_step->duration_ms) {
		//On valide le step
		switch (pattern->execution_state){
			case PATTERN_EXEC_RUNNING:
				current_state->state = OUTPUT_STEP_COMPLETED;
				break;
			case PATTERN_EXEC_PAUSED:
				current_state->state = OUTPUT_STEP_SKIP;
				break;
			default:
				break;
		}

		//Dans les deux cas
		//On pointe le suivant si possible
		if (pattern->current_step_id + 1 < pattern->state_buffer->length){
			pattern->current_step_id ++;
		} 
		else
		{
			//On regarde si des cycles sont encore à réaliser
			if (pattern->cycles_done < pattern->cycle_target) {
				pattern->cycles_done ++;
				pattern->current_step_id = 0;
			}
			else
			{
				//le pattern est complet
				pattern->execution_state = PATTERN_EXEC_COMPLETED;

				event = instant_event(pattern,OUTPUT_KEEP);

				return event;
			}
		}

		current_state = &pattern->state_buffer->data[pattern->current_step_id];

		current_state->state = OUTPUT_STEP_RUNNING;
		current_state->step_start_ms = now_ms;

    }

	

}

// On transmet la séquence à exécuter
bool output_pattern_run(OutputPattern_t *pattern, const OutputPatternSequence_t *sequence, OutputPatternStateBuffer_t *state_buffer, uint16_t cycle_target, bool abort_on_fail, TargetState_t state_on_fail, time_ms_t now_ms)
{
	if (pattern == NULL ||
		sequence == NULL ||
		sequence->steps == NULL ||
		state_buffer == NULL ||
		sequence->length == 0 ||
		state_buffer->length != sequence->length ||
		pattern->current_step_id < pattern->state_buffer->length)
	{
		return false;
	}

	pattern->sequence = sequence;
	pattern->state_buffer = state_buffer;
	pattern->cycle_target = cycle_target;
	pattern->abort_on_fail = abort_on_fail;
	pattern->state_on_fail = state_on_fail;
	output_pattern_reset(pattern);

	//On active le pattern
	OutputPatternStepState_t *current_state = &pattern->state_buffer->data[pattern->current_step_id];
	current_state->step_start_ms = now_ms;
	current_state->state = OUTPUT_STEP_RUNNING;
	pattern->execution_state = PATTERN_EXEC_RUNNING;



	return true;
}
//si loop -> reset auto
//sinon mise en pause


// Reset : réinitialise tous les step de la séquence en cours (steps en pending) + mise en pause
void output_pattern_reset(OutputPattern_t *pattern)
{
	if (pattern == NULL || pattern->state_buffer == NULL) return;
	
	pattern->execution_state = PATTERN_EXEC_STOPPED;
	pattern->cycles_done = 0;
	pattern->flag_any_failed = false;
	for (int i = 0; i<pattern->state_buffer->length; ++i) 
	{
		pattern->state_buffer->data->state = OUTPUT_STEP_PENDING;
		pattern->state_buffer->data->step_start_ms = 0;
	}
}




// Pause / Reprise

OutputPatternEvent_t output_pattern_stop(OutputPattern_t *pattern,TargetState_t output_state, time_ms_t now_ms)
{
	OutputPatternEvent_t event = instant_event(pattern, output_state);

	//On appelle update pour valider la position où doit être figée le pattern
	if (pattern != NULL && pattern->execution_state == PATTERN_EXEC_RUNNING) event = output_pattern_update(pattern, now_ms);


	pattern->execution_state = PATTERN_EXEC_STOPPED;
	event.pattern_state = PATTERN_EXEC_STOPPED;

	//On utilise le state du step actuel pour mémoriser le temps déjà écoulé dans le step
	time_ms_t old_delta = pattern->state_buffer->data[pattern->current_step_id].step_start_ms;
	old_delta = (time_ms_t)(now_ms - old_delta);
	pattern->state_buffer->data[pattern->current_step_id].step_start_ms = old_delta;
	return event;
}

OutputPatternEvent_t output_pattern_pause(OutputPattern_t *pattern,TargetState_t output_state, time_ms_t now_ms)
{

	// On déclenche un update pour ne pas skipper un step qui serait terminé au moment de l'appel à pause
	OutputPatternEvent_t event = output_pattern_update(pattern, now_ms);

	pattern->execution_state = PATTERN_EXEC_PAUSED;
	event.pattern_state = PATTERN_EXEC_PAUSED;

	return event;
}


OutputPatternEvent_t output_pattern_resume(OutputPattern_t *pattern, time_ms_t now_ms)
{
	OutputPatternEvent_t event = instant_event(pattern, OUTPUT_KEEP);

	if (pattern == NULL ) return event;

    if (output_pattern_is_completed(pattern))
    {
        pattern->execution_state = PATTERN_EXEC_COMPLETED;
    }else
    {
		// Si le pattern était en stopped, on doit mettre à jour le timer du stepState en cours
		//On récupère le temps déjà écoulé (et sauvegardé) dans le step_start_ms du state du step
		time_ms_t old_delta = pattern->state_buffer->data[pattern->current_step_id].step_start_ms;
		if (pattern->execution_state == PATTERN_EXEC_STOPPED) pattern->state_buffer->data[pattern->current_step_id].step_start_ms = (time_ms_t)(now_ms - old_delta);
        pattern->execution_state = PATTERN_EXEC_RUNNING;
    }

	pattern->execution_state = PATTERN_EXEC_
}


// Indique si tous les steps sont traités (la séquence est complète, il reste peut-être des loop à réaliser)
bool output_pattern_all_completed(const OutputPattern_t *pattern)
{

}

// Indique si le dernier pattern a été traité
bool output_pattern_loop_over(const OutputPattern_t *pattern)
{

} // retourne false si loop en cours, true si la dernière loop est terminée

// Indique si le pattern est terminé
bool output_pattern_is_completed(const OutputPattern_t *pattern)
{
    if (pattern == NULL) return false;

    if (pattern->cycle_target == 0)
        return false; // Cycles illimités

    return pattern->cycles_done >= pattern->cycle_target;
}

// Indique l'état d'un pattern spécifique
OutputPatternEvent_t output_pattern_state(const OutputPattern_t *pattern, uint8_t step_id)
{

}




static OutputPatternEvent_t instant_event(OutputPattern_t *pattern,TargetState_t output_state )
{
	OutputPatternEvent_t event = {0};
	
	if (pattern == NULL ) return event;

	event.pattern_state = pattern->execution_state;
	event.current_step = pattern->current_step_id;
	event.cycles_done = pattern->cycles_done;
	event.new_failed = pattern->flag_any_failed;
	event.new_skiped = pattern->flag_any_skiped;

	pattern->flag_any_failed = false;
	pattern->flag_any_skiped = false;

	if (pattern->output == NULL) return event;

	if (output_state != OUTPUT_KEEP) pattern->output->state = (output_state == OUTPUT_ON);
	
	return event;
}