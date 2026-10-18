#include "libpic/input.h"
#include "test_helpers.h"

static state_t g_pin_state = STATE_LOW;

static state_t mock_read(uint16_t pin_id)
{
    (void)pin_id;
    return g_pin_state;
}

int main(void)
{
    input_t input;
    input_event_t event;

    /* Entrée active à l'état haut, anti-rebond de 10 ms. */
    g_pin_state = STATE_LOW;
    input_init(&input, mock_read, 0, true, 10);
    TEST_ASSERT(!input_is_active(&input));

    /* Aucun changement : aucun événement. */
    event = input_update(&input, 0);
    TEST_ASSERT(!event.rising_edge);
    TEST_ASSERT(!event.falling_edge);
    TEST_ASSERT(!input_is_active(&input));

    /* Changement brut, mais anti-rebond non écoulé. */
    g_pin_state = STATE_HIGH;
    event = input_update(&input, 5);
    TEST_ASSERT(!event.rising_edge);
    TEST_ASSERT(!input_is_active(&input));

    /* Rebonds : chaque changement repousse l'échéance de l'anti-rebond. */
    g_pin_state = STATE_LOW;
    event = input_update(&input, 8);
    TEST_ASSERT(!event.rising_edge);
    g_pin_state = STATE_HIGH;
    event = input_update(&input, 9);
    TEST_ASSERT(!event.rising_edge);
    TEST_ASSERT(!input_is_active(&input));

    /* Délai écoulé depuis le dernier changement : l'entrée devient active. */
    event = input_update(&input, 20);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(event.rising_edge);
    TEST_ASSERT(!event.falling_edge);

    /* Trop de temps depuis l'appel précédent : l'événement n'est pas fiable. */
    TEST_ASSERT(!event.reliable);

    /* État stable : plus aucun front. */
    event = input_update(&input, 21);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(!event.rising_edge);
    TEST_ASSERT(!event.falling_edge);

    /* Front descendant après anti-rebond. */
    g_pin_state = STATE_LOW;
    event = input_update(&input, 22);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(!event.falling_edge);

    event = input_update(&input, 35);
    TEST_ASSERT(!input_is_active(&input));
    TEST_ASSERT(event.falling_edge);
    TEST_ASSERT(!event.rising_edge);

    /* Un front détecté avec des appels plus rapprochés que la durée
       d'anti-rebond est fiable. */
    input_init(&input, mock_read, 0, true, 10);
    g_pin_state = STATE_LOW;
    (void)input_update(&input, 100);
    g_pin_state = STATE_HIGH;
    (void)input_update(&input, 101);
    (void)input_update(&input, 110);
    event = input_update(&input, 111);
    TEST_ASSERT(event.rising_edge);
    TEST_ASSERT(event.reliable);

    /* active_high = false : l'état actif correspond à STATE_LOW. */
    g_pin_state = STATE_LOW;
    input_init(&input, mock_read, 0, false, 0);
    event = input_update(&input, 200);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(event.rising_edge);

    printf("test_input: OK\n");
    return TEST_RESULT();
}
