#include <stdbool.h>
#include <stddef.h>
#include "input_group.h"
#include "input_pattern.h"

bool input_group_create(InputGroup_t *group)
{
    if (group == NULL) return 0;

    group->count = 0;
    for (uint8_t i = 0; i < INPUT_GROUP_MAX; i++) {
        group->inputs[i] = NULL;
        group->matchers[i] = NULL;
    }

    return 1;
}

bool input_group_add_input(InputGroup_t *group, Input_t *input)
{
    if (group == NULL || input == NULL) return false;
    if (group->count >= INPUT_GROUP_MAX) return false;
    
    group->inputs[group->count] = input;
    group->matchers[group->count] = NULL;   // ← pas de matcher
    group->count++;
    
    return true;
}

bool input_group_add_matcher(InputGroup_t *group, MatcherPatternGlobal_t *matcher)
{
    if (group == NULL || matcher == NULL || matcher->input == NULL) return false;
    if (group->count >= INPUT_GROUP_MAX) return false;

    group->inputs[group->count] = matcher->input;   // ← on récupère l'input depuis le matcher
    group->matchers[group->count] = matcher;
    group->count++;

    return true;
}

void input_group_update_all(InputGroup_t *group, uint16_t ticks)
{
    if (group == NULL) return;

    for (uint8_t i = 0; i < group->count; i++) {
        if (group->inputs[i] != NULL) {
            // Si un matcher est associé, c'est lui qui gère l'input
            // (il appelle input_update en interne)
            if (group->matchers[i] != NULL) {
                pattern_matcher_update(group->matchers[i], ticks);
            }
            // Sinon, on met à jour l'input directement
            else {
                input_update(group->inputs[i], ticks);
            }
        }
    }
}

uint32_t input_group_get_state(const InputGroup_t *group)
{
    uint32_t state = 0;

    if (group == NULL) return 0;

    for (uint8_t i = 0; i < group->count; i++) {
        if (group->inputs[i] != NULL && input_is_active(group->inputs[i])) {
            state |= (1UL << i);
        }
    }

    return state;
}