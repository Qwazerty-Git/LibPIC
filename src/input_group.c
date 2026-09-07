#include "libpic/input_group.h"
#include <stddef.h>

void input_group_init(input_group_t *group, input_t **inputs, uint8_t count)
{
    if (group == NULL) {
        return;
    }

    group->inputs = inputs;
    group->count = count;
}

void input_group_update(input_group_t *group, time_ms_t now_ms)
{
    uint8_t i;

    if (group == NULL || group->inputs == NULL) {
        return;
    }

    for (i = 0; i < group->count; i++) {
        if (group->inputs[i] != NULL) {
            input_update(group->inputs[i], now_ms);
        }
    }
}

bool input_group_any_active(const input_group_t *group)
{
    return input_group_active_count(group) > 0;
}

bool input_group_all_active(const input_group_t *group)
{
    if (group == NULL || group->inputs == NULL || group->count == 0) {
        return false;
    }

    return input_group_active_count(group) == group->count;
}

uint8_t input_group_active_count(const input_group_t *group)
{
    uint8_t i;
    uint8_t count = 0;

    if (group == NULL || group->inputs == NULL) {
        return 0;
    }

    for (i = 0; i < group->count; i++) {
        if (group->inputs[i] != NULL && input_is_active(group->inputs[i])) {
            count++;
        }
    }

    return count;
}

input_t *input_group_get(const input_group_t *group, uint8_t index)
{
    if (group == NULL || group->inputs == NULL || index >= group->count) {
        return NULL;
    }

    return group->inputs[index];
}
