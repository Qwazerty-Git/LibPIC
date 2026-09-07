#include "libpic/timer.h"
#include "test_helpers.h"

int main(void)
{
    timer_t timer;

    /* One-shot timer. */
    timer_init(&timer, 100, false);
    TEST_ASSERT(!timer_is_running(&timer));

    timer_start(&timer, 0);
    TEST_ASSERT(timer_is_running(&timer));
    TEST_ASSERT(!timer_update(&timer, 50));
    TEST_ASSERT(timer_elapsed(&timer, 50) == 50);
    TEST_ASSERT(timer_remaining(&timer, 50) == 50);

    TEST_ASSERT(timer_update(&timer, 100));
    TEST_ASSERT(!timer_is_running(&timer));
    TEST_ASSERT(timer_remaining(&timer, 100) == 0);

    /* Once stopped, a one-shot timer no longer reports expiry. */
    TEST_ASSERT(!timer_update(&timer, 200));

    /* Auto reload timer keeps firing periodically. */
    timer_init(&timer, 10, true);
    timer_start(&timer, 0);
    TEST_ASSERT(timer_update(&timer, 10));
    TEST_ASSERT(timer_is_running(&timer));
    TEST_ASSERT(!timer_update(&timer, 15));
    TEST_ASSERT(timer_update(&timer, 20));

    timer_stop(&timer);
    TEST_ASSERT(!timer_is_running(&timer));
    TEST_ASSERT(!timer_update(&timer, 30));

    /* Auto reload timer resynchronizes in one step after missed periods,
       instead of requiring one timer_update call per missed period. */
    timer_init(&timer, 10, true);
    timer_start(&timer, 0);
    TEST_ASSERT(timer_update(&timer, 55));
    TEST_ASSERT(!timer_update(&timer, 56));
    TEST_ASSERT(timer_update(&timer, 60));

    printf("test_timer: OK\n");
    return TEST_RESULT();
}
