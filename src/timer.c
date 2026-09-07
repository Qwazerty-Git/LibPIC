#include "libpic/timer.h"

void timer_init(timer_t *timer, time_ms_t duration_ms, bool auto_reload)
{
    if (timer == NULL) {
        return;
    }

    timer->duration_ms = duration_ms;
    timer->start_time_ms = 0;
    timer->running = false;
    timer->auto_reload = auto_reload;
}

void timer_start(timer_t *timer, time_ms_t now_ms)
{
    if (timer == NULL) {
        return;
    }

    timer->start_time_ms = now_ms;
    timer->running = true;
}

void timer_stop(timer_t *timer)
{
    if (timer == NULL) {
        return;
    }

    timer->running = false;
}

bool timer_update(timer_t *timer, time_ms_t now_ms)
{
    if (timer == NULL || !timer->running) {
        return false;
    }

    if ((time_ms_t)(now_ms - timer->start_time_ms) >= timer->duration_ms) {
        if (timer->auto_reload && timer->duration_ms > 0) {
            /* Resynchronize in one step even if several periods have been
               missed (e.g. timer_update was not called for a while), instead
               of only advancing by a single period per call. */
            time_ms_t elapsed_periods = (time_ms_t)(now_ms - timer->start_time_ms) / timer->duration_ms;
            timer->start_time_ms += elapsed_periods * timer->duration_ms;
        } else if (!timer->auto_reload) {
            timer->running = false;
        }
        return true;
    }

    return false;
}

bool timer_is_running(const timer_t *timer)
{
    if (timer == NULL) {
        return false;
    }

    return timer->running;
}

time_ms_t timer_elapsed(const timer_t *timer, time_ms_t now_ms)
{
    if (timer == NULL || !timer->running) {
        return 0;
    }

    return (time_ms_t)(now_ms - timer->start_time_ms);
}

time_ms_t timer_remaining(const timer_t *timer, time_ms_t now_ms)
{
    time_ms_t elapsed;

    if (timer == NULL || !timer->running) {
        return 0;
    }

    elapsed = timer_elapsed(timer, now_ms);

    if (elapsed >= timer->duration_ms) {
        return 0;
    }

    return (time_ms_t)(timer->duration_ms - elapsed);
}
