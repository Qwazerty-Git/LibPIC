#include "libpic/output_group.h"
#include "test_helpers.h"

static state_t g_pin_a = STATE_LOW;
static state_t g_pin_b = STATE_LOW;

static void write_a(state_t state) { g_pin_a = state; }
static void write_b(state_t state) { g_pin_b = state; }

int main(void)
{
    output_t output_a;
    output_t output_b;
    output_t *outputs[2];
    output_group_t group;

    output_init(&output_a, write_a, true);
    output_init(&output_b, write_b, true);

    outputs[0] = &output_a;
    outputs[1] = &output_b;

    output_group_init(&group, outputs, 2);

    output_group_set_all(&group, true);
    TEST_ASSERT(output_is_active(&output_a));
    TEST_ASSERT(output_is_active(&output_b));
    TEST_ASSERT(g_pin_a == STATE_HIGH);
    TEST_ASSERT(g_pin_b == STATE_HIGH);

    output_group_set_index(&group, 1, false);
    TEST_ASSERT(output_is_active(&output_a));
    TEST_ASSERT(!output_is_active(&output_b));

    output_group_toggle_all(&group);
    TEST_ASSERT(!output_is_active(&output_a));
    TEST_ASSERT(output_is_active(&output_b));

    TEST_ASSERT(output_group_get(&group, 0) == &output_a);
    TEST_ASSERT(output_group_get(&group, 2) == NULL);

    printf("test_output_group: OK\n");
    return TEST_RESULT();
}
