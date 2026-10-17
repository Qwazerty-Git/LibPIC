#include "libpic/input_pattern.h"
#include <stddef.h>

bool input_pattern_matcher_init(MatcherInputPattern_t *matcher,
                        input_t *input,
                        const MatcherInputPatternList_t *pattern_list,
                        MatcherInputPatternState_t *states,
                        bool paused)
{
    if (matcher == NULL ||
        input == NULL ||
        pattern_list == NULL ||
        pattern_list->patterns == NULL ||
        states == NULL) {
        return false;
    }

    matcher->input = input;
    matcher->list = pattern_list;
    matcher->states = states;
    matcher->paused = paused;

    bool any_error = false;

    if (states != NULL && pattern_list != NULL) 
    {
        for (uint8_t i = 0; i < pattern_list->count; i++) 
        {
            const MatcherInputPatternSequence_t *pattern = &matcher->list->patterns[i];
            
            states[i].step_index = 0;
            states[i].step_start_ms = 0;
            states[i].state = IPS_ACTIVE;
            states[i].error_config = false;

            if (pattern->length == 0 || pattern->steps == NULL) {
                states[i].state = IPS_INACTIVE;
                states[i].error_config = true;
                any_error = true;
                continue;
            }

            for (uint8_t j = 0; j < pattern->length; j++) {
                if (pattern->steps[j].max_ms != PATTERN_UNLIMITED_MS &&
                    pattern->steps[j].max_ms < pattern->steps[j].min_ms) {
                    states[i].state = IPS_INACTIVE;
                    states[i].error_config = true;
                    any_error = true;
                    break;
                }
            }     

        }
    }
    return !any_error;
}

void input_pattern_matcher_reset(MatcherInputPattern_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        matcher->states[i].step_index = 0;
        matcher->states[i].step_start_ms = 0;
        if (!matcher->states[i].error_config) matcher->states[i].state = IPS_ACTIVE;
        //matcher->states[i].error_config = false; // On conserve les erreurs de configuration précédentes
    }
}

void input_pattern_matcher_pause(MatcherInputPattern_t *matcher)
{
    if (matcher == NULL) return;
    matcher->paused = true;
}

void input_pattern_matcher_resume(MatcherInputPattern_t *matcher)
{
    if (matcher == NULL) return;
    matcher->paused = false;
}

bool input_pattern_matcher_all_inactive(const MatcherInputPattern_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return true;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        if (matcher->states[i].state != IPS_INACTIVE) {
            return false;
        }
    }
    return true;
}

bool input_pattern_matcher_is_matched(const MatcherInputPattern_t *matcher, uint8_t id)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return false;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        if (matcher->list->patterns[i].id == id) {
            return matcher->states[i].state & PATTERN_IS_MATCH_MASK;
        }
    }
    return false;
}

MatcherInputPatternResult_t input_pattern_matcher_update(MatcherInputPattern_t *matcher, time_ms_t now_ms)
{
    MatcherInputPatternResult_t result = {false};

    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL || !matcher->input) {
        return result;
    }

    // Si en pause, on ne traite pas
    if (matcher->paused) {
        return result;
    }

    // On déclenche l'update de l'input affilié
    input_event_t event = input_update(matcher->input, now_ms);

    // Traiter chaque pattern
    for (uint8_t i = 0; i < matcher->list->count; i++) {

        MatcherInputPatternState_t *state = &matcher->states[i];
        const MatcherInputPatternSequence_t *pattern = &matcher->list->patterns[i];

        // On s'assure que les patterns en erreur de configuration sont ignorés
        if (state->error_config) {
            state->state = IPS_INACTIVE;
            continue;
        }

        // On bascule les just_matched de l'update précédent en matched
        if (state->state == IPS_JUST_MATCHED) {
            state->state = IPS_MATCHED;
        }    
        
        // Ignorer les patterns déjà invalidés ou déjà validés
        if (state->state == IPS_INACTIVE || state->state == IPS_MATCHED) continue;

        // Déterminer l'état attendu pour l'étape actuelle
        // pair (0, 2, 4) = appui (1), impair (1, 3, 5) = relâchement (0)
        bool expected_event = (state->step_index & 1u)? event.rising_edge : event.falling_edge;
        bool has_event = event.rising_edge || event.falling_edge;
        bool event_is_unreliable = has_event && !event.reliable;
        
        // Initialiser step_start_ms si on est au tout début et qu'un appui vient de se produire
        if (state->step_index == 0) 
        {
            if (event.rising_edge && !event.reliable) {
                state->state = IPS_INACTIVE;
                continue;
            }

            if (!event.rising_edge) {
                continue;
            }

            state->step_start_ms = now_ms;
            state->state = IPS_PENDING;
            result.all_inactive = false;
            continue;
        } 
        else if ((!expected_event && has_event) || event_is_unreliable) 
        {
            state->state = IPS_INACTIVE;
            continue;
        }

        // On calcule le temps écoulé depuis le début de l'étape actuelle
        time_ms_t elapsed_ms = (time_ms_t)(now_ms - state->step_start_ms);

        const InputPatternStep_t *step =&pattern->steps[state->step_index];
        bool is_last_step = state->step_index == (uint8_t)(pattern->length - 1);
        bool can_match_by_time = is_last_step && step->max_ms == PATTERN_UNLIMITED_MS && elapsed_ms >= step->min_ms;
        
        if (step->max_ms != PATTERN_UNLIMITED_MS && elapsed_ms > step->max_ms) {
            state->state = IPS_INACTIVE;
        }
        else if (has_event) {
            if (elapsed_ms < step->min_ms) {
                state->state = IPS_INACTIVE;
            }
            else {
                state->step_index++;

                if (state->step_index >= pattern->length) {
                    state->state = IPS_JUST_MATCHED;
                    result.any_matched = true;
                } else {
                    state->step_start_ms = now_ms;
                    state->state = IPS_PENDING;
                }
            }
        } 
        else if (can_match_by_time) {
            state->state = IPS_JUST_MATCHED;
            result.any_matched = true;
        }

        if (state->state != IPS_INACTIVE) {
            result.all_inactive = false;
        }
    }


    return result;
}