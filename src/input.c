#include "libpic/input.h"

void input_init(input_t *input, read_pin_fn read, uint16_t pin_id, bool active_high, time_ms_t debounce_delay_ms)
{
    if (input == NULL) {
        return;
    }

    input->read = read;
    input->active_high = active_high;
    input->debounce_delay_ms = debounce_delay_ms;
    input->raw_state = active_high ? STATE_LOW : STATE_HIGH;
    input->state = false;
    input->last_change_time = 0;
    input->pin_id = pin_id;
    input->last_call_time = 0;
}

input_event_t input_update(input_t *input, time_ms_t now_ms)
{
    state_t raw;
    bool logical_raw;

    if (input == NULL || input->read == NULL) {
        return (input_event_t){0};
    }

    raw = input->read(input->pin_id);

    bool previous_state = input->state;

    if (raw != input->raw_state) {
        input->raw_state = raw;
        input->last_change_time = now_ms;
    }

    if ((time_ms_t)(now_ms - input->last_change_time) >= input->debounce_delay_ms) {
        logical_raw = input->active_high ? (raw == STATE_HIGH) : (raw == STATE_LOW);
        input->state = logical_raw;
    }

    time_ms_t delta = (time_ms_t)(now_ms - input->last_call_time);
    input->last_call_time = now_ms;
    bool reliable = (delta <= input->debounce_delay_ms);


    return (input_event_t){
        .rising_edge = input->state && !previous_state,
        .falling_edge = !input->state && previous_state,
        // La fiabilité est bonne si on n'a pas "sauté" le debounce
        .reliable = ((input->state && !previous_state) || (!input->state && previous_state)) && reliable
    };
}

bool input_is_active(const input_t *input)
{
    if (input == NULL) {
        return false;
    }

    return input->state;
}

