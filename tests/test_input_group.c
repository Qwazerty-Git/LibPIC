#include "libpic/input_group.h"
#include "test_helpers.h"

static state_t g_pin_a = STATE_LOW;
static state_t g_pin_b = STATE_LOW;

static state_t read_a(void) { return g_pin_a; }
static state_t read_b(void) { return g_pin_b; }

int main(void)
{
    input_t input_a;
    input_t input_b;
    input_t *inputs[2];
    input_group_t group;

    input_init(&input_a, read_a, true, 0);
    input_init(&input_b, read_b, true, 0);

    inputs[0] = &input_a;
    inputs[1] = &input_b;

    input_group_init(&group, inputs, 2);

    input_group_update(&group, 0);
    TEST_ASSERT(!input_group_any_active(&group));
    TEST_ASSERT(!input_group_all_active(&group));
    TEST_ASSERT(input_group_active_count(&group) == 0);

    g_pin_a = STATE_HIGH;
    input_group_update(&group, 1);
    TEST_ASSERT(input_group_any_active(&group));
    TEST_ASSERT(!input_group_all_active(&group));
    TEST_ASSERT(input_group_active_count(&group) == 1);

    g_pin_b = STATE_HIGH;
    input_group_update(&group, 2);
    TEST_ASSERT(input_group_all_active(&group));
    TEST_ASSERT(input_group_active_count(&group) == 2);

    TEST_ASSERT(input_group_get(&group, 0) == &input_a);
    TEST_ASSERT(input_group_get(&group, 1) == &input_b);
    TEST_ASSERT(input_group_get(&group, 2) == NULL);

    printf("test_input_group: OK\n");
    return TEST_RESULT();
}
