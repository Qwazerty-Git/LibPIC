#include "libpic/output.h"

void output_init(output_t *output, write_pin_fn write,uint16_t id_pin, bool active_high, bool initial_state)
{
    if (output == NULL) return;

    output->write = write;
    output->active_high = active_high;
    output->pin_id = id_pin;

    output_set(output, initial_state);
}

void output_set(output_t *output, bool active)
{
    state_t pin_state;

    if (output == NULL) return;
    
    output->state = active;

    if (output->write != NULL) {
        pin_state = (active == output->active_high) ? STATE_HIGH : STATE_LOW;
        output->write(output->pin_id, pin_state);
    }
}

void output_off(output_t *output)
{
    if (output == NULL) return;
    output_set(output, false);
}

void output_on(output_t *output)
{
    if (output == NULL) return;
    output_set(output, true);
}

void output_toggle(output_t *output)
{
    if (output == NULL) return;
    output_set(output, !output->state);
}

bool output_is_active(const output_t *output)
{
    if (output == NULL) return false;
    return output->state;
}
