#include "libpic/output.h"
#include "test_helpers.h"

static state_t g_last_written = STATE_LOW;
static uint16_t g_last_pin = 0xFFFFu;
static int g_write_count = 0;

static void mock_write(uint16_t pin_id, state_t state)
{
    g_last_pin = pin_id;
    g_last_written = state;
    g_write_count++;
}

int main(void)
{
    output_t output;

    output_init(&output, mock_write, 7, true, false);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);
    TEST_ASSERT(g_last_pin == 7);

    output_set(&output, true);
    TEST_ASSERT(output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_HIGH);

    output_toggle(&output);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    output_on(&output);
    TEST_ASSERT(output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_HIGH);

    output_off(&output);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    /* active_high = false : l'état actif écrit STATE_LOW sur la broche. */
    output_init(&output, mock_write, 7, false, true);
    TEST_ASSERT(output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    output_set(&output, false);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_HIGH);

    TEST_ASSERT(g_write_count > 0);

    printf("test_output: OK (%d writes)\n", g_write_count);
    return TEST_RESULT();
}
