#include "libpic/output_group.h"

void output_group_init(output_group_t *group, output_t **outputs, uint8_t count)
{
    if (group == NULL) {
        return;
    }

    group->outputs = outputs;
    group->count = count;
}

void output_group_set_all(output_group_t *group, bool active)
{
    uint8_t i;

    if (group == NULL || group->outputs == NULL) {
        return;
    }

    for (i = 0; i < group->count; i++) {
        if (group->outputs[i] != NULL) {
            output_set(group->outputs[i], active);
        }
    }
}

void output_group_toggle_all(output_group_t *group)
{
    uint8_t i;

    if (group == NULL || group->outputs == NULL) {
        return;
    }

    for (i = 0; i < group->count; i++) {
        if (group->outputs[i] != NULL) {
            output_toggle(group->outputs[i]);
        }
    }
}

void output_group_set_index(output_group_t *group, uint8_t index, bool active)
{
    output_t *output = output_group_get(group, index);

    if (output != NULL) {
        output_set(output, active);
    }
}

output_t *output_group_get(const output_group_t *group, uint8_t index)
{
    if (group == NULL || group->outputs == NULL || index >= group->count) {
        return NULL;
    }

    return group->outputs[index];
}
