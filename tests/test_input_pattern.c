#include "libpic/input_pattern.h"
#include "test_helpers.h"

static state_t g_pin_state = STATE_LOW;

static state_t mock_read(uint16_t pin_id)
{
    (void)pin_id;
    return g_pin_state;
}

/* Clic court : appui de 0 à 500 ms, terminé par le relâchement. */
static const InputPatternStep_t short_click_steps[] = {
    { 0, 500 }
};

/* Appui long : 2000 ms minimum, reconnu sans attendre le relâchement. */
static const InputPatternStep_t long_press_steps[] = {
    { 2000, PATTERN_UNLIMITED_MS }
};

static const MatcherInputPatternSequence_t patterns[] = {
    { short_click_steps, 1 },
    { long_press_steps,  1 }
};

static const MatcherInputPatternList_t pattern_list = {
    patterns, 2
};

/* Configuration invalide : max_ms < min_ms. */
static const InputPatternStep_t invalid_steps[] = {
    { 500, 100 }
};

static const MatcherInputPatternSequence_t invalid_patterns[] = {
    { invalid_steps, 1 }
};

static const MatcherInputPatternList_t invalid_list = {
    invalid_patterns, 1
};

int main(void)
{
    input_t button;
    MatcherInputPattern_t matcher;
    MatcherInputPatternState_t states[2];
    MatcherInputPatternResult_t result;
    MatcherInputPattern_t bad_matcher;
    MatcherInputPatternState_t bad_states[1];

    /* Anti-rebond de 10 ms et appels toutes les 10 ms -> événements fiables. */
    g_pin_state = STATE_LOW;
    input_init(&button, mock_read, 0, true, 10);
    TEST_ASSERT(input_pattern_matcher_init(&matcher, &button, &pattern_list, states,
                                           (uint8_t)(sizeof(states) / sizeof(states[0])),
                                           false));

    /* --- clic court --- */

    result = input_pattern_matcher_update(&matcher, 0);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_ACTIVE);

    /* Changement brut : l'anti-rebond n'est pas encore écoulé. */
    g_pin_state = STATE_HIGH;
    result = input_pattern_matcher_update(&matcher, 10);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_ACTIVE);

    /* Front montant fiable : l'appui démarre les deux patterns. */
    result = input_pattern_matcher_update(&matcher, 20);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_PENDING);
    TEST_ASSERT(states[1].state == IPS_PENDING);

    /* Relâchement : d'abord le changement brut... */
    g_pin_state = STATE_LOW;
    result = input_pattern_matcher_update(&matcher, 30);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_PENDING);

    /* ... puis le front descendant, fiable après l'anti-rebond. */
    result = input_pattern_matcher_update(&matcher, 40);
    TEST_ASSERT(result.any_matched);
    TEST_ASSERT(states[0].state == IPS_JUST_MATCHED);
    TEST_ASSERT(input_pattern_matcher_is_matched(&matcher, 0));
    TEST_ASSERT(!input_pattern_matcher_is_matched(&matcher, 1));

    /* L'état IPS_JUST_MATCHED devient IPS_MATCHED à la mise à jour suivante. */
    result = input_pattern_matcher_update(&matcher, 50);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_MATCHED);
    TEST_ASSERT(input_pattern_matcher_is_matched(&matcher, 0));

    TEST_ASSERT(!input_pattern_matcher_all_inactive(&matcher));

    /* Remise à zéro : tous les patterns valides redeviennent actifs. */
    input_pattern_matcher_reset(&matcher);
    TEST_ASSERT(states[0].state == IPS_ACTIVE);
    TEST_ASSERT(states[1].state == IPS_ACTIVE);
    TEST_ASSERT(!input_pattern_matcher_is_matched(&matcher, 0));

    /* --- appui long --- */

    g_pin_state = STATE_HIGH;
    result = input_pattern_matcher_update(&matcher, 1010);
    TEST_ASSERT(!result.any_matched);
    result = input_pattern_matcher_update(&matcher, 1020);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_PENDING);
    TEST_ASSERT(states[1].state == IPS_PENDING);

    /* Au-delà de 500 ms, le pattern de clic court devient impossible. */
    result = input_pattern_matcher_update(&matcher, 1540);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_INACTIVE);
    TEST_ASSERT(states[1].state == IPS_PENDING);

    /* À 2000 ms d'appui, le pattern long est reconnu sans relâchement. */
    result = input_pattern_matcher_update(&matcher, 3020);
    TEST_ASSERT(result.any_matched);
    TEST_ASSERT(states[1].state == IPS_JUST_MATCHED);
    TEST_ASSERT(input_pattern_matcher_is_matched(&matcher, 1));
    TEST_ASSERT(!input_pattern_matcher_is_matched(&matcher, 0));

    /* --- pause --- */

    g_pin_state = STATE_LOW;
    (void)input_pattern_matcher_update(&matcher, 3030);
    input_pattern_matcher_reset(&matcher);
    input_pattern_matcher_pause(&matcher);

    g_pin_state = STATE_HIGH;
    (void)input_pattern_matcher_update(&matcher, 20000);
    (void)input_pattern_matcher_update(&matcher, 20010);
    g_pin_state = STATE_LOW;
    result = input_pattern_matcher_update(&matcher, 20020);
    TEST_ASSERT(!result.any_matched);
    TEST_ASSERT(states[0].state == IPS_ACTIVE);
    TEST_ASSERT(states[1].state == IPS_ACTIVE);

    input_pattern_matcher_resume(&matcher);

    /* --- configuration invalide --- */

    TEST_ASSERT(!input_pattern_matcher_init(&bad_matcher, &button, &invalid_list, bad_states,
                                            (uint8_t)(sizeof(bad_states) / sizeof(bad_states[0])),
                                            false));
    TEST_ASSERT(bad_states[0].error_config);
    TEST_ASSERT(bad_states[0].state == IPS_INACTIVE);
    TEST_ASSERT(input_pattern_matcher_all_inactive(&bad_matcher));

    (void)input_pattern_matcher_update(&bad_matcher, 21000);
    TEST_ASSERT(bad_states[0].state == IPS_INACTIVE);

    /* L'erreur de configuration est conservée après une remise à zéro. */
    input_pattern_matcher_reset(&bad_matcher);
    TEST_ASSERT(bad_states[0].error_config);
    TEST_ASSERT(bad_states[0].state == IPS_INACTIVE);
    TEST_ASSERT(input_pattern_matcher_all_inactive(&bad_matcher));

    printf("test_input_pattern: OK\n");
    return TEST_RESULT();
}
