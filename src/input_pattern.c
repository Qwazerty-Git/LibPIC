#include "input_pattern.h"
#include <stddef.h>

void pattern_matcher_init(MatcherPatternGlobal_t *matcher,
                        Input_t *input,
                        const MatcherPatternList_t *pattern_list,
                        MatcherPatternState_t *states,
                        bool paused)
{
    if (matcher == NULL) return;

    matcher->list = pattern_list;
    matcher->states = states;
    matcher->paused = paused;

    if (states != NULL && pattern_list != NULL) {
        for (uint8_t i = 0; i < pattern_list->count; i++) {
            states[i].step_index = 0;
            states[i].step_ticks = 0;
            states[i].active = true;
            states[i].matched = false;
        }
    }
}

void pattern_matcher_reset(MatcherPatternGlobal_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        matcher->states[i].step_index = 0;
        matcher->states[i].step_ticks = 0;
        matcher->states[i].active = true;
        matcher->states[i].matched = false;
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
        if (matcher->states[i].active) {
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
            return matcher->states[i].matched;
        }
    }
    return false;
}

MatcherPatternResult_t pattern_matcher_update(MatcherPatternGlobal_t *matcher, uint16_t ticks)
{
    MatcherPatternResult_t result = {false};

    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL || !matcher->input) {
        return result;
    }

    // Si en pause, on ne traite pas
    if (matcher->paused) {
        return result;
    }

    // Reset des résultats du précédent update
    for (uint8_t i = 0; i < matcher->list->count; i++) {
        matcher->states[i].matched = false;
    }

    // Lire l'état actuel de l'input (après debounce, avec active_high appliqué)
    bool current_state = input_is_active(matcher->input);

    // Traiter chaque pattern
    for (uint8_t i = 0; i < matcher->list->count; i++) {
        MatcherPatternState_t *state = &matcher->states[i];
        const MatcherPattern_t *pattern = &matcher->list->patterns[i];

        // Reset des résultats du précédent update
        state->matched = false;

        // Ignorer les patterns déjà invalidés
        if (!state->active) continue;

        // Déterminer l'état attendu pour l'étape actuelle
        // pair (0, 2, 4) = appui (1), impair (1, 3, 5) = relâchement (0)
        bool expected_state = (state->step_index % 2 == 0);

        // Cas particulier : au tout début (étape 0), on attend le premier appui
        // Si l'état actuel est 0 (relâché), on ne fait rien
        if (state->step_index == 0 && !current_state) {
            // On attend le premier appui, il ne s'est pas encore produit
            continue;
        }

        // Vérifier la correspondance avec l'état actuel
        if (current_state == expected_state) {
            // L'état correspond, on incrémente le temps
            state->step_ticks += ticks;

            // Vérifier le dépassement de durée
            if (state->step_ticks > pattern->steps[state->step_index]) {
                // Durée dépassée → pattern invalide
                state->active = false;
                state->matched = false;
                continue;
            }

            // Vérifier si on a atteint la durée maximale (fin d'étape)
            if (state->step_ticks == pattern->steps[state->step_index]) {
                // Passer à l'étape suivante
                state->step_index++;
                state->step_ticks = 0;

                // Si toutes les étapes sont terminées → PATTERN VALIDÉ
                if (state->step_index >= pattern->length) {
                    state->matched = true;
                    result.any_matched = true;
                    state->active = false;  // Le pattern est terminé
                }
            }
        } else {
            // L'état ne correspond pas
            // On attendait un appui, on a un relâchement → invalide
            // Ou on attendait un relâchement, on a un appui → invalide
            state->active = false;
            state->matched = false;
        }
    }

    // Auto-reset si tous les patterns sont invalides
    if (pattern_matcher_all_inactive(matcher)) {
        pattern_matcher_reset(matcher);
    }

    return result;
}