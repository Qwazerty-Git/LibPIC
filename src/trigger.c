#include "libpic/trigger.h"

void trigger_init(trigger_t *trigger, trigger_condition_fn condition, trigger_callback_fn callback, void *context)
{
    if (trigger == NULL) {
        return;
    }

    trigger->condition = condition;
    trigger->callback = callback;
    trigger->context = context;
    trigger->previous_state = false;
    trigger->enabled = true;
}

void trigger_enable(trigger_t *trigger, bool enable)
{
    if (trigger == NULL) {
        return;
    }

    trigger->enabled = enable;

    if (!enable) {
        trigger->previous_state = false;
    }
}

bool trigger_is_enabled(const trigger_t *trigger)
{
    if (trigger == NULL) {
        return false;
    }

    return trigger->enabled;
}

bool trigger_update(trigger_t *trigger)
{
    bool current_state;
    bool fired;

    if (trigger == NULL || !trigger->enabled || trigger->condition == NULL) {
        return false;
    }

    current_state = trigger->condition(trigger->context);
    fired = current_state && !trigger->previous_state;
    trigger->previous_state = current_state;

    if (fired && trigger->callback != NULL) {
        trigger->callback(trigger->context);
    }

    return fired;
}
