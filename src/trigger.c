	/*
	Note sur l'utilisation de PatternList :
	La variable TriggerPatternList_t doit être déclarée en static pour que le pointeur reste en vie
	tant qu'il est utilisé dans l'instance de Trigger. Elle doit rester en vie pendant toute
	la durée d'utilisation du trigger initialisé avec elle.

	Exemple :
	static const uint16_t pattern_values[] = {100, 200, 300};
	static const TriggerPatternList_t pattern = {
		.values = (uint16_t*)pattern_values,
		.length = 3
	};
	*/

#include "trigger.h"
#include <stddef.h>

// Fonctions statiques internes
static bool trigger_init_error(Trigger_t *trigger);
static TriggerEvent_t gestion_mode_cycle(Trigger_t *trigger, TimerEvent_t timer_e);
static TriggerEvent_t gestion_mode_pattern(Trigger_t *trigger, TimerEvent_t timer_e);
static TriggerEvent_t gestion_mode_sync(Trigger_t *trigger);
static TriggerEvent_t timer_to_trigger_event(const TimerEvent_t *timer_e,const  TriggerEvent_t *trigger_e, uint16_t pattern_index);

// Fonction de création
bool trigger_create(Trigger_t *trigger)
{
    if (trigger == NULL) {
        return false;
    }

    *trigger = (Trigger_t){0};

    return true;
}

// Fonction d'enregistrement du callback utilisateur
void trigger_set_receiver(Trigger_t *trigger, TriggerReceiver receiver, void *context)
{
    if (trigger == NULL) return;

    trigger->receiver = receiver;
    trigger->receiver_context = context;
}

// Fonctions d'initialisation
bool trigger_init_cycle(Trigger_t *trigger, uint16_t period, uint16_t nbr_cycles)
{
    if (!trigger || period == 0) {
        return trigger_init_error(trigger);
    }
    trigger->mode = TRIGGER_CYCLE;
    timer_init(&trigger->timer, period, 0);

    return true;
}

bool trigger_init_pattern(Trigger_t *trigger, const TriggerPatternList_t *pattern, uint16_t nbr_cycles)
{
    if (!trigger || !pattern || pattern->length == 0) {
        return trigger_init_error(trigger);
    }
    trigger->mode = TRIGGER_PATTERN;
    trigger->pattern = pattern;

    uint16_t first_duration = pattern->values[0];

    timer_init(&trigger->timer, first_duration, 0);
    trigger->pattern_index = 0;

    return true;
}

bool trigger_init_sync(Trigger_t *trigger, const bool *sync_state)
{
    if (!trigger || !sync_state) {
        return trigger_init_error(trigger);
    }
    trigger->mode = TRIGGER_SYNC;
    trigger->sync_last_state = *sync_state;
    trigger->sync_state = sync_state;

    return true;
}

// Fonction de mise à jour principale
TriggerEvent_t trigger_update(Trigger_t *trigger, uint16_t ticks)
{
    if (trigger == NULL) return (TriggerEvent_t){0};

    // Si le trigger est en pause, on ne fait rien
    if (trigger_is_paused(trigger))
    {
        return (TriggerEvent_t){.event = false, .iteration = trigger->timer.iteration,
                                .pattern_index = trigger->pattern_index, .iteration_overflow = false};
    }

    // On transmet les ticks au timer interne
    TimerEvent_t timer_e = timer_update(&trigger->timer, ticks);

    // On délègue le traitement selon le mode
    switch (trigger->mode)
    {
        case TRIGGER_CYCLE:
            return gestion_mode_cycle(trigger, timer_e);
        case TRIGGER_PATTERN:
            return gestion_mode_pattern(trigger, timer_e);
        case TRIGGER_SYNC:
            return gestion_mode_sync(trigger);
        case TRIGGER_NONE:
        default:
            return timer_to_trigger_event(&timer_e,NULL, trigger->pattern_index);
    }
}

// Fonctions de contrôle
void trigger_reset(Trigger_t *trigger)
{
    if (trigger == NULL) return;

    timer_reset(&trigger->timer);
    trigger->pattern_index = 0;

    if (trigger->mode == TRIGGER_SYNC &&
        trigger->sync_state != NULL)
    {
        trigger->sync_last_state = *trigger->sync_state;
    }
}

void trigger_pause(Trigger_t *trigger)
{
    if (trigger == NULL) return;

    timer_pause(&trigger->timer);
}

TriggerEvent_t trigger_resume(Trigger_t *trigger)
{
    if (trigger == NULL) return (TriggerEvent_t){0};

    TimerEvent_t timer_e = timer_resume(&trigger->timer);

    switch (trigger->mode)
    {
        case TRIGGER_CYCLE:
            return gestion_mode_cycle(trigger, timer_e);
        case TRIGGER_PATTERN:
            return gestion_mode_pattern(trigger, timer_e);
        case TRIGGER_SYNC:
            return gestion_mode_sync(trigger);
        case TRIGGER_NONE:
        default:
            return timer_to_trigger_event(&timer_e,NULL, trigger->pattern_index);
    }
}

// Fonctions d'accès
TriggerMode trigger_get_mode(const Trigger_t *trigger)
{
    if (trigger == NULL) return TRIGGER_NONE;
    return trigger->mode;
}

uint16_t trigger_get_pattern_index(const Trigger_t *trigger)
{
    if (trigger == NULL) return 0;
    return trigger->pattern_index;
}

bool trigger_is_paused(const Trigger_t *trigger)
{
    if (trigger == NULL) return true;
    return timer_is_paused(&trigger->timer);
}

bool trigger_cycle_over(const Trigger_t *trigger)
{
    if (trigger == NULL) return true;
    return timer_cycle_over(&trigger->timer);
}

bool trigger_is_sync(const Trigger_t *trigger)
{
    if (trigger == NULL) return false;
    return (trigger->mode == TRIGGER_SYNC && trigger->sync_state != NULL && !trigger_is_paused(trigger));
}

// Fonctions statiques internes

static bool trigger_init_error(Trigger_t *trigger)
{
    if (trigger) {
        trigger->mode = TRIGGER_NONE;
    }
    return false;
}

static TriggerEvent_t gestion_mode_cycle(Trigger_t *trigger, TimerEvent_t timer_e)
{
    TriggerEvent_t trigger_e = timer_to_trigger_event(&timer_e, &trigger_e, trigger->pattern_index);

    if (timer_e.event == false) return trigger_e;

    if (timer_e.iteration & 1) { // Impaire = OFF (fin de cycle)
        if (trigger->cycle_max != 0) { //Non cycle infini
            trigger->cycle_count--;

            if (trigger->cycle_count == 0) {
                trigger_e.cycle_over = true;
                timer_pause(&trigger->timer);
            }

        }
    }

    // On émet un callback si existant
    if (trigger->receiver != NULL) {
        trigger->receiver(trigger->receiver_context, trigger_e);
    }

    return trigger_e;
}

static TriggerEvent_t gestion_mode_pattern(Trigger_t *trigger, TimerEvent_t timer_e)
{
    TriggerEvent_t trigger_e = timer_to_trigger_event(&timer_e, &trigger_e, trigger->pattern_index);

    if (timer_e.event == false) return trigger_e;

    //On regarde le pattern qui vient de se terminer
    // On cherche le dernier pattern du dernier cycle
    if ((trigger->pattern_index +1 == trigger->pattern->length) && (trigger->cycle_max != 0) && (trigger->cycle_count == 1)) { 
        trigger_e.cycle_over = true;
        timer_pause(&trigger->timer);
    } else {
        // On passe au pattern suivant
        trigger->pattern_index++;
        if (trigger->pattern_index >= trigger->pattern->length) {
            trigger->pattern_index = 0;

            uint16_t next_duration = trigger->pattern->values[trigger->pattern_index];
            timer_set_duration(&trigger->timer, next_duration);            

            if (trigger->cycle_max != 0) {
                trigger->cycle_count--;
            }     
        }   
    }


    // On émet un callback pour l'évènement actuel
    if (trigger->receiver != NULL) {
        trigger->receiver(trigger->receiver_context, trigger_e);
    }

    return trigger_e;
}

static TriggerEvent_t gestion_mode_sync(Trigger_t *trigger)
{
    TriggerEvent_t trigger_e = (TriggerEvent_t){0};

    if (trigger->sync_state == NULL) {
        trigger->mode = TRIGGER_NONE;
        return trigger_e;
    }

    // On ne vérifie le changement d'état
    if (*trigger->sync_state != trigger->sync_last_state) {

        trigger_e.event = true;

        // On émet un callback si existant
        if (trigger->receiver != NULL) {
            trigger->receiver(trigger->receiver_context, trigger_e);
        }

        // On mémorise le nouvel état
        trigger->sync_last_state = *trigger->sync_state;
    }

    return trigger_e;
}

static TriggerEvent_t timer_to_trigger_event(const TimerEvent_t *timer_e,const  TriggerEvent_t *trigger_e, uint16_t pattern_index)
{
    if (!timer_e) return (TriggerEvent_t){0};

	TriggerEvent_t trigger_bis;
	
	if (trigger_e != NULL) {
		trigger_bis= *trigger_e;
	} else {
		trigger_bis = (TriggerEvent_t){0};
	}
	
    trigger_bis.event = timer_e->event;
    trigger_bis.iteration = timer_e->iteration;
    trigger_bis.iteration_overflow = timer_e->iteration_overflow;
    trigger_bis.cycle_over = timer_e->cycle_over;
    trigger_bis.pattern_index = pattern_index;

	return trigger_bis;
}
