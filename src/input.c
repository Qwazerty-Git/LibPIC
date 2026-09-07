#include "libpic/input.h"

void input_init(input_t *input, read_pin_fn read, bool active_high, time_ms_t debounce_delay_ms)
{
    if (input == NULL) {
        return;
    }

    input->read = read;
    input->active_high = active_high;
    input->debounce_delay_ms = debounce_delay_ms;
    input->raw_state = active_high ? STATE_LOW : STATE_HIGH;
    input->state = false;
    input->previous_state = false;
    input->last_change_time = 0;
}

void input_update(input_t *input, time_ms_t now_ms)
{
    state_t raw;
    bool logical_raw;

    if (input == NULL || input->read == NULL) {
        return;
    }

    raw = input->read();
    input->previous_state = input->state;

    if (raw != input->raw_state) {
        input->raw_state = raw;
        input->last_change_time = now_ms;
    }

    if ((time_ms_t)(now_ms - input->last_change_time) >= input->debounce_delay_ms) {
        logical_raw = input->active_high ? (raw == STATE_HIGH) : (raw == STATE_LOW);
        input->state = logical_raw;
    }
}

bool input_is_active(const input_t *input)
{
    if (input == NULL) {
        return false;
    }

    return input->state;
}

bool input_rising_edge(const input_t *input)
{
    if (input == NULL) {
        return false;
    }

    return input->state && !input->previous_state;
}

bool input_falling_edge(const input_t *input)
{
    if (input == NULL) {
        return false;
    }

    return !input->state && input->previous_state;
}
