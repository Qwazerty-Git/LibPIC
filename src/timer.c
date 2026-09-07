#include "timer.h"
#include <stddef.h>

bool timer_create(Timer_t *timer)
{
    if (timer == NULL) {
        return false;
    }

    *timer = (Timer_t){0};

    return true;
}

bool timer_init(Timer_t *timer, uint16_t duration, uint16_t cycle)
{
    if (timer == NULL || duration == 0) return false;
    timer_reset(timer);
    timer->duration = duration;
    timer->nbr_cycles = cycle;

    return true;
}

void timer_set_receiver(Timer_t *timer, TimerReceiver receiver, void *context)
{
    if (timer == NULL) return;

    timer->receiver = receiver;
    timer->receiver_context = context;
}

TimerEvent_t timer_update(Timer_t *timer, uint16_t ticks)
{
    if (timer == NULL) return (TimerEvent_t){0};

    TimerEvent_t timer_e = (TimerEvent_t)
    {
        .event = false,
        .iteration = timer->iteration,
        .iteration_overflow = false,
        .cycle_over = timer_cycle_over(timer)
    };

    // On incrémente uniquement si le timer n'est pas en pause et a une durée valide
    if (timer->paused || timer->duration == 0)
    {
        return timer_e;
    }

    timer->counter += ticks;

    if (timer->counter >= timer->duration) {

        while (timer->counter >= timer->duration) {
            timer->counter -= timer->duration;
            timer->iteration++;
            if (timer->iteration == 0) timer_e.iteration_overflow = true;

            if (timer_cycle_over(timer)) {
                timer->paused = true;
                break;
            }
        }

        timer_e.event = true;
        timer_e.iteration = timer->iteration;
        timer_e.cycle_over = timer_cycle_over(timer);

        if (timer->receiver != NULL) {
            timer->receiver(timer->receiver_context, timer_e);
        }

        return timer_e;
    }

    return timer_e;
}

bool timer_set_duration(Timer_t *timer, uint16_t duration)
{
    if (timer == NULL || duration == 0) return false;

    timer->duration = duration;
    timer->counter = 0;

    return true;
}

void timer_reset(Timer_t *timer)
{
    if (timer == NULL) return;
    timer->iteration = 0;
    timer->counter = 0;
    timer->paused = false;
}

void timer_pause(Timer_t *timer)
{
    if (timer == NULL) return;
    timer->paused = true;
}

TimerEvent_t timer_resume(Timer_t *timer)
{
    if (timer == NULL) return (TimerEvent_t){0};
    timer->paused = false;

    if (timer_cycle_over(timer))
    {
        timer->iteration = 0;
        timer->counter = 0;
    }

    TimerEvent_t timer_e = timer_update(timer, 0);

    if (timer->receiver != NULL && timer_e.event) {
        timer->receiver(timer->receiver_context, timer_e);
    }

    return timer_e;
}

bool timer_is_paused(const Timer_t *timer)
{
    if (timer == NULL) return true;
    return timer->paused;
}

bool timer_cycle_over(const Timer_t *timer)
{
    if (timer == NULL) return true;
    return (timer->nbr_cycles > 0 && timer->iteration >= timer->nbr_cycles);
}