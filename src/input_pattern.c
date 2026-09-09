#include "libpic/input_pattern.h"
#include <stddef.h>

bool pattern_matcher_init(MatcherPatternGlobal_t *matcher,
                        input_t *input,
                        const MatcherPatternList_t *pattern_list,
                        MatcherPatternState_t *states,
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
            const MatcherPattern_t *pattern = &matcher->list->patterns[i];
            
            states[i].step_index = 0;
            states[i].step_start_ms = 0;
            states[i].state = PS_ACTIVE;
            states[i].error_config = false;

            if (pattern->length == 0 || pattern->steps == NULL) {
                states[i].state = PS_INACTIVE;
                states[i].error_config = true;
                any_error = true;
                continue;
            }

            for (uint8_t j = 0; j < pattern->length; j++) {
                if (pattern->steps[j].max_ms != PATTERN_UNLIMITED_MS &&
                    pattern->steps[j].max_ms < pattern->steps[j].min_ms) {
                    states[i].state = PS_INACTIVE;
                    states[i].error_config = true;
                    any_error = true;
                    break;
                }
            }     

        }
    }
    return !any_error;
}

void pattern_matcher_reset(MatcherPatternGlobal_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        matcher->states[i].step_index = 0;
        matcher->states[i].step_start_ms = 0;
        if (!matcher->states[i].error_config) matcher->states[i].state = PS_ACTIVE;
        //matcher->states[i].error_config = false; // On conserve les erreurs de configuration précédentes
    }
}

void pattern_matcher_pause(MatcherPatternGlobal_t *matcher)
{
    if (matcher == NULL) return;
    matcher->paused = true;
}

void pattern_matcher_resume(MatcherPatternGlobal_t *matcher)
{
    if (matcher == NULL) return;
    matcher->paused = false;
}

bool pattern_matcher_all_inactive(const MatcherPatternGlobal_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return true;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        if (matcher->states[i].state != PS_INACTIVE) {
            return false;
        }
    }
    return true;
}

bool pattern_matcher_is_matched(const MatcherPatternGlobal_t *matcher, uint8_t id)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return false;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        if (matcher->list->patterns[i].id == id) {
            return matcher->states[i].state & PATTERN_IS_MATCH_MASK;
        }
    }
    return false;
}

MatcherPatternResult_t pattern_matcher_update(MatcherPatternGlobal_t *matcher, time_ms_t now_ms)
{
    MatcherPatternResult_t result = {false};

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

        MatcherPatternState_t *state = &matcher->states[i];
        const MatcherPattern_t *pattern = &matcher->list->patterns[i];

        // On s'assure que les patterns en erreur de configuration sont ignorés
        if (state->error_config) {
            state->state = PS_INACTIVE;
            continue;
        }

        // On bascule les just_matched de l'update précédent en matched
        if (state->state == PS_JUST_MATCHED) {
            state->state = PS_MATCHED;
        }    
        
        // Ignorer les patterns déjà invalidés ou déjà validés
        if (state->state == PS_INACTIVE || state->state == PS_MATCHED) continue;

        // Déterminer l'état attendu pour l'étape actuelle
        // pair (0, 2, 4) = appui (1), impair (1, 3, 5) = relâchement (0)
        bool expected_event = (state->step_index & 1u)? event.rising_edge : event.falling_edge;
        bool has_event = event.rising_edge || event.falling_edge;
        bool event_is_unreliable = has_event && !event.reliable;
        
        // Initialiser step_start_ms si on est au tout début et qu'un appui vient de se produire
        if (state->step_index == 0) 
        {
            if (event.rising_edge && !event.reliable) {
                state->state = PS_INACTIVE;
                continue;
            }

            if (!event.rising_edge) {
                continue;
            }

            state->step_start_ms = now_ms;
            state->state = PS_PENDING;
            result.all_inactive = false;
            continue;
        } 
        else if ((!expected_event && has_event) || event_is_unreliable) 
        {
            state->state = PS_INACTIVE;
            continue;
        }

        // On calcule le temps écoulé depuis le début de l'étape actuelle
        time_ms_t elapsed_ms = (time_ms_t)(now_ms - state->step_start_ms);

        const PatternStep_t *step =&pattern->steps[state->step_index];
        bool is_last_step = state->step_index == (uint8_t)(pattern->length - 1);
        bool can_match_by_time = is_last_step && step->max_ms == PATTERN_UNLIMITED_MS && elapsed_ms >= step->min_ms;
        
        if (step->max_ms != PATTERN_UNLIMITED_MS && elapsed_ms > step->max_ms) {
            state->state = PS_INACTIVE;
        }
        else if (has_event) {
            if (elapsed_ms < step->min_ms) {
                state->state = PS_INACTIVE;
            }
            else {
                state->step_index++;

                if (state->step_index >= pattern->length) {
                    state->state = PS_JUST_MATCHED;
                    result.any_matched = true;
                } else {
                    state->step_start_ms = now_ms;
                    state->state = PS_PENDING;
                }
            }
        } 
        else if (can_match_by_time) {
            state->state = PS_JUST_MATCHED;
            result.any_matched = true;
        }

        if (state->state != PS_INACTIVE) {
            result.all_inactive = false;
        }
    }


    return result;
}