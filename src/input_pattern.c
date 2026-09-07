#include "libpic/input_pattern.h"

void input_pattern_init(input_pattern_t *pattern, input_t *input,
                         time_ms_t long_press_threshold_ms, time_ms_t double_press_window_ms)
{
    if (pattern == NULL) {
        return;
    }

    pattern->input = input;
    pattern->long_press_threshold_ms = long_press_threshold_ms;
    pattern->double_press_window_ms = double_press_window_ms;
    pattern->press_start_time = 0;
    pattern->last_release_time = 0;
    pattern->pressed = false;
    pattern->awaiting_second_press = false;
}

input_pattern_result_t input_pattern_update(input_pattern_t *pattern, time_ms_t now_ms)
{
    input_pattern_result_t result = INPUT_PATTERN_NONE;
    time_ms_t press_duration;

    if (pattern == NULL || pattern->input == NULL) {
        return INPUT_PATTERN_NONE;
    }

    if (input_rising_edge(pattern->input)) {
        pattern->pressed = true;
        pattern->press_start_time = now_ms;
    } else if (input_falling_edge(pattern->input)) {
        pattern->pressed = false;
        press_duration = (time_ms_t)(now_ms - pattern->press_start_time);

        if (press_duration >= pattern->long_press_threshold_ms) {
            pattern->awaiting_second_press = false;
            result = INPUT_PATTERN_LONG_PRESS;
        } else if (pattern->awaiting_second_press &&
                   (time_ms_t)(now_ms - pattern->last_release_time) <= pattern->double_press_window_ms) {
            pattern->awaiting_second_press = false;
            result = INPUT_PATTERN_DOUBLE_PRESS;
        } else {
            pattern->awaiting_second_press = true;
            pattern->last_release_time = now_ms;
        }
    } else if (!pattern->pressed && pattern->awaiting_second_press &&
               (time_ms_t)(now_ms - pattern->last_release_time) > pattern->double_press_window_ms) {
        pattern->awaiting_second_press = false;
        result = INPUT_PATTERN_SHORT_PRESS;
    }

    return result;
}
