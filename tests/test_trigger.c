#include "libpic/trigger.h"
#include "test_helpers.h"

static bool g_condition = false;
static int g_callback_count = 0;

static bool condition_fn(void *context)
{
    (void)context;
    return g_condition;
}

static void callback_fn(void *context)
{
    int *counter = (int *)context;
    (*counter)++;
}

int main(void)
{
    trigger_t trigger;

    trigger_init(&trigger, condition_fn, callback_fn, &g_callback_count);
    TEST_ASSERT(trigger_is_enabled(&trigger));

    TEST_ASSERT(!trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 0);

    g_condition = true;
    TEST_ASSERT(trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 1);

    /* Condition remains true: no re-trigger until it goes false again. */
    TEST_ASSERT(!trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 1);

    g_condition = false;
    TEST_ASSERT(!trigger_update(&trigger));

    g_condition = true;
    TEST_ASSERT(trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 2);

    /* Disabling the trigger prevents further triggering. */
    trigger_enable(&trigger, false);
    TEST_ASSERT(!trigger_is_enabled(&trigger));
    g_condition = false;
    TEST_ASSERT(!trigger_update(&trigger));
    g_condition = true;
    TEST_ASSERT(!trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 2);

    trigger_enable(&trigger, true);
    g_condition = false;
    TEST_ASSERT(!trigger_update(&trigger));
    g_condition = true;
    TEST_ASSERT(trigger_update(&trigger));
    TEST_ASSERT(g_callback_count == 3);

    printf("test_trigger: OK\n");
    return TEST_RESULT();
}
