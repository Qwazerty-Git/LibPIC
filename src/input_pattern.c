#include "libpic/input_pattern.h"
#include <stddef.h>

bool input_pattern_matcher_init(MatcherInputPattern_t *matcher,
                        input_t *input,
                        const MatcherInputPatternList_t *pattern_list,
                        MatcherInputPatternState_t *states,
                        uint8_t states_count,
                        bool paused)
{
    if (matcher == NULL ||
        input == NULL ||
        pattern_list == NULL ||
        pattern_list->patterns == NULL ||
        states == NULL ||
        states_count != pattern_list->count) {
        return false;
    }

    matcher->input = input;
    matcher->list = pattern_list;
    matcher->states = states;
    matcher->paused = paused;

    // Parcours par pointeurs plutôt que par indices : sur un petit coeur 8 bits,
    // states[i] et patterns[i] imposent une multiplication par la taille de la
    // structure à chaque tour.
    const MatcherInputPatternSequence_t *pattern = pattern_list->patterns;
    MatcherInputPatternState_t *state = states;
    bool config_ok = true;

    for (uint8_t i = 0; i < pattern_list->count; i++) {

        bool error = false;

#if LIBPIC_CHECK_CONFIG
        const InputPatternStep_t *step = pattern->steps;
        uint8_t length = pattern->length;
        // Une séquence sans étape n'a aucun sens : elle ne peut jamais aboutir
        error = (length == 0) || (step == NULL);

        // Une durée maximale non nulle doit rester >= min_ms
        for (uint8_t j = 0; !error && j < length; j++) {
            if (step->max_ms != PATTERN_UNLIMITED_MS && step->max_ms < step->min_ms) {
                error = true;
            }
            step++;
        }
#endif

        state->step_index = 0;
        state->step_start_ms = 0;
        state->error_config = error;
        state->state = error ? IPS_INACTIVE : IPS_ACTIVE;
        config_ok = config_ok && !error;

        pattern++;
        state++;
    }

    return config_ok;
}

void input_pattern_matcher_reset(MatcherInputPattern_t *matcher)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return;

    MatcherInputPatternState_t *state = matcher->states;

    for (uint8_t i = 0; i < matcher->list->count; i++) {
        state->step_index = 0;
        state->step_start_ms = 0;
        // Les erreurs de configuration sont conservées d'une remise à zéro à
        // l'autre : un pattern invalide le reste.
        if (!state->error_config) state->state = IPS_ACTIVE;
        state++;
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

bool input_pattern_matcher_is_matched(const MatcherInputPattern_t *matcher, uint8_t index)
{
    if (matcher == NULL || matcher->states == NULL || matcher->list == NULL) return false;
    if (index >= matcher->list->count) return false;

    return (matcher->states[index].state & PATTERN_IS_MATCH_MASK) != 0;
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

    // Ces deux valeurs ne dépendent pas du pattern : on les calcule une fois
    // plutôt qu'à chaque tour de boucle.
    bool has_event = event.rising_edge || event.falling_edge;
    bool unreliable = has_event && !event.reliable;

    const MatcherInputPatternSequence_t *pattern = matcher->list->patterns;
    MatcherInputPatternState_t *state = matcher->states;

    for (uint8_t i = 0; i < matcher->list->count; i++) {

        uint8_t exec_state = state->state;

        // Reconnu lors de l'update précédent : on le fige et on n'y touche plus.
        // Le bit de poids fort couvre IPS_JUST_MATCHED et IPS_MATCHED.
        if (exec_state & PATTERN_IS_MATCH_MASK) {
            state->state = IPS_MATCHED;
            pattern++;
            state++;
            continue;
        }

        // Erreur de configuration ou pattern invalidé : rien à faire
        if (state->error_config || exec_state == IPS_INACTIVE) {
            pattern++;
            state++;
            continue;
        }

        // Déterminer l'état attendu pour l'étape actuelle
        // pair (0, 2, 4) = appui (1), impair (1, 3, 5) = relâchement (0)
        bool expected_event = (state->step_index & 1u) ? event.rising_edge : event.falling_edge;

        // Le pattern attend son premier appui : seul un rising_edge le démarre
        if (exec_state == IPS_ACTIVE) {
            if (event.rising_edge) {
                state->step_start_ms = now_ms;
                state->state = event.reliable ? IPS_PENDING : IPS_INACTIVE;
            }
            pattern++;
            state++;
            continue;
        }

        if ((!expected_event && has_event) || unreliable) {
            state->state = IPS_INACTIVE;
            pattern++;
            state++;
            continue;
        }

        // On calcule le temps écoulé depuis le début de l'étape actuelle
        time_ms_t elapsed_ms = (time_ms_t)(now_ms - state->step_start_ms);
        const InputPatternStep_t *step = &pattern->steps[state->step_index];

        if (step->max_ms != PATTERN_UNLIMITED_MS && elapsed_ms > step->max_ms) {
            state->state = IPS_INACTIVE;
        }
        else if (has_event) {
            if (elapsed_ms < step->min_ms) {
                state->state = IPS_INACTIVE;
            } else {
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
        else if (state->step_index == (uint8_t)(pattern->length - 1) &&
                 step->max_ms == PATTERN_UNLIMITED_MS &&
                 elapsed_ms >= step->min_ms) {
            // Dernière étape sans limite de durée : l'appui long est reconnu
            // dès que min_ms est atteint, sans attendre le relâchement.
            state->state = IPS_JUST_MATCHED;
            result.any_matched = true;
        }

        pattern++;
        state++;
    }

    return result;
}