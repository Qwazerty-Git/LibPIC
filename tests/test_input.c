#include "libpic/input.h"
#include "test_helpers.h"

static state_t g_pin_state = STATE_LOW;

static state_t mock_read(void)
{
    return g_pin_state;
}

int main(void)
{
    input_t input;

    input_init(&input, mock_read, true, 10);
    TEST_ASSERT(!input_is_active(&input));

    /* No change yet: still inactive. */
    input_update(&input, 0);
    TEST_ASSERT(!input_is_active(&input));

    /* Pin goes high, but debounce delay not elapsed yet. */
    g_pin_state = STATE_HIGH;
    input_update(&input, 5);
    TEST_ASSERT(!input_is_active(&input));

    /* Still bouncing (noise) before debounce elapses: should not matter,
       last_change_time resets. */
    g_pin_state = STATE_LOW;
    input_update(&input, 8);
    TEST_ASSERT(!input_is_active(&input));

    g_pin_state = STATE_HIGH;
    input_update(&input, 9);
    TEST_ASSERT(!input_is_active(&input));

    /* Debounce delay elapsed after the last transition: input becomes active. */
    input_update(&input, 20);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(input_rising_edge(&input));
    TEST_ASSERT(!input_falling_edge(&input));

    /* Steady state: no more edges reported. */
    input_update(&input, 21);
    TEST_ASSERT(input_is_active(&input));
    TEST_ASSERT(!input_rising_edge(&input));

    /* Pin goes low and stays low past the debounce delay. */
    g_pin_state = STATE_LOW;
    input_update(&input, 22);
    input_update(&input, 35);
    TEST_ASSERT(!input_is_active(&input));
    TEST_ASSERT(input_falling_edge(&input));

    /* active_high = false: active state corresponds to STATE_LOW. */
    input_init(&input, mock_read, false, 0);
    g_pin_state = STATE_LOW;
    input_update(&input, 0);
    TEST_ASSERT(input_is_active(&input));

    printf("test_input: OK\n");
    return TEST_RESULT();
}
