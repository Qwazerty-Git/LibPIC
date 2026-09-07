#include "libpic/output.h"
#include "test_helpers.h"

static state_t g_last_written = STATE_LOW;
static int g_write_count = 0;

static void mock_write(state_t state)
{
    g_last_written = state;
    g_write_count++;
}

int main(void)
{
    output_t output;

    output_init(&output, mock_write, true);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    output_set(&output, true);
    TEST_ASSERT(output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_HIGH);

    output_toggle(&output);
    TEST_ASSERT(!output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    /* active_high = false: active state writes STATE_LOW to the pin. */
    output_init(&output, mock_write, false);
    output_set(&output, true);
    TEST_ASSERT(output_is_active(&output));
    TEST_ASSERT(g_last_written == STATE_LOW);

    printf("test_output: OK (%d writes)\n", g_write_count);
    return TEST_RESULT();
}
