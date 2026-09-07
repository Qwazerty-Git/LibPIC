#include "libpic/input_pattern.h"
#include "test_helpers.h"

static state_t g_pin_state = STATE_LOW;

static state_t mock_read(void)
{
    return g_pin_state;
}

int main(void)
{
    input_t button;
    input_pattern_t pattern;

    input_init(&button, mock_read, true, 0);
    input_pattern_init(&pattern, &button, 500, 300);

    /* Short press: press then release quickly, then wait past the double
       press window without a second press -> SHORT_PRESS reported. */
    g_pin_state = STATE_HIGH;
    input_update(&button, 0);
    TEST_ASSERT(input_pattern_update(&pattern, 0) == INPUT_PATTERN_NONE);

    g_pin_state = STATE_LOW;
    input_update(&button, 100);
    TEST_ASSERT(input_pattern_update(&pattern, 100) == INPUT_PATTERN_NONE);

    input_update(&button, 500);
    TEST_ASSERT(input_pattern_update(&pattern, 500) == INPUT_PATTERN_SHORT_PRESS);

    /* Long press: held longer than the threshold. */
    g_pin_state = STATE_HIGH;
    input_update(&button, 1000);
    TEST_ASSERT(input_pattern_update(&pattern, 1000) == INPUT_PATTERN_NONE);

    g_pin_state = STATE_LOW;
    input_update(&button, 1600);
    TEST_ASSERT(input_pattern_update(&pattern, 1600) == INPUT_PATTERN_LONG_PRESS);

    /* Double press: two short presses within the double press window. */
    g_pin_state = STATE_HIGH;
    input_update(&button, 2000);
    input_pattern_update(&pattern, 2000);

    g_pin_state = STATE_LOW;
    input_update(&button, 2050);
    TEST_ASSERT(input_pattern_update(&pattern, 2050) == INPUT_PATTERN_NONE);

    g_pin_state = STATE_HIGH;
    input_update(&button, 2150);
    input_pattern_update(&pattern, 2150);

    g_pin_state = STATE_LOW;
    input_update(&button, 2200);
    TEST_ASSERT(input_pattern_update(&pattern, 2200) == INPUT_PATTERN_DOUBLE_PRESS);

    printf("test_input_pattern: OK\n");
    return TEST_RESULT();
}
