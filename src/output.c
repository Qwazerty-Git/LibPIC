#include "output.h"
#include <stddef.h>

bool output_create(Output_t *output, volatile uint8_t *port, uint8_t mask, bool active_high, bool default_state)
{
    if (output == NULL) return false;

    output->mapping.port = port;
    output->mapping.mask = mask;
    output->active_high = active_high;
    output->enabled = true;
    output_set_state(output, default_state);

    return true;
}

void output_on(Output_t *output)
{
    if (output == NULL || !output->enabled) return;
    output->state = true;
    output_update_gpio(output);
}

void output_off(Output_t *output)
{
    if (output == NULL || !output->enabled) return;
    output->state = false;
    output_update_gpio(output);
}

bool output_toggle(Output_t *output)
{
    if (output == NULL || !output->enabled) return false;
    output->state = !output->state;
    output_update_gpio(output);
    return output->state;
}

void output_set_state(Output_t *output, bool state)
{
    if (output == NULL || !output->enabled) return;
    output->state = state;
    output_update_gpio(output);
}

void output_enable(Output_t *output, bool enable)
{
    if (output == NULL) return;
    output->enabled = enable;
}

void output_update_gpio(Output_t *output)
{
    if (output == NULL || output->mapping.port == NULL || !output->enabled) return;

    bool physical_state = output->state ^ !output->active_high;

    if (physical_state) {
        *output->mapping.port |= output->mapping.mask;
    } else {
        *output->mapping.port &= ~output->mapping.mask;
    }
}

bool output_is_on(const Output_t *output)
{
    if (output == NULL) return false;
    return output->state;
}

bool output_is_physically_high(const Output_t *output)
{
    if (output == NULL) return false;
    return (output->state ^ !output->active_high);
}