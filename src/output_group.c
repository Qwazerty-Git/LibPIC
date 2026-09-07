#include "output_group.h"
#include <stddef.h>

static void output_group_apply_value(OutputGroup_t *group, uint32_t value);

bool output_group_create(OutputGroup_t *group)
{
    if (group == NULL) return false;

    group->count = 0;
    for (uint8_t i = 0; i < OUTPUT_GROUP_MAX; i++) {
        group->outputs[i] = NULL;
    }

    return true;
}

int8_t output_group_add(OutputGroup_t *group, Output_t *output)
{
    if (group == NULL || output == NULL) return -1;
    if (group->count >= OUTPUT_GROUP_MAX) return -1;

    group->outputs[group->count] = output;
    return group->count++;
}

Output_t *output_group_get(const OutputGroup_t *group, uint8_t index)
{
    if (group == NULL || index >= group->count) return NULL;
    return group->outputs[index];
}

void output_group_update_gpio_all(OutputGroup_t *group)
{
    if (group == NULL) return;

    for (uint8_t i = 0; i < group->count; i++) {
        if (group->outputs[i] != NULL) {
            output_update_gpio(group->outputs[i]);
        }
    }
}

void output_group_receive(OutputGroup_t *group, uint32_t value)
{
    output_group_apply_value(group, value);
}

void output_group_set_state_all(OutputGroup_t *group, bool state)
{
    output_group_apply_value(group, state ? 0xFFFFFFFF : 0x00000000);
}

void output_group_enable_all(OutputGroup_t *group, bool enable)
{
    if (group == NULL) return;

    for (uint8_t i = 0; i < group->count; i++) {
        if (group->outputs[i] != NULL) {
            output_enable(group->outputs[i], enable);
        }
    }
}

void output_group_enable_mask(OutputGroup_t *group, uint32_t mask)
{
    if (group == NULL) return;

    for (uint8_t i = 0; i < group->count && i < 32; i++) {
        if (group->outputs[i] != NULL) {
            output_enable(group->outputs[i], (mask >> i) & 0x1);
        }
    }
}


// Fonction interne : applique value aux outputs
static void output_group_apply_value(OutputGroup_t *group, uint32_t value)
{
    if (group == NULL) return;

    for (uint8_t i = 0; i < group->count && i < 32; i++) {
        if (group->outputs[i] != NULL) {
            output_set_state(group->outputs[i], (value >> i) & 0x1);
        }
    }
}