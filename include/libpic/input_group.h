#ifndef INPUT_GROUP_H
#define INPUT_GROUP_H

#include <stdbool.h>
#include <stdint.h>
#include "input.h"
#include "input_pattern.h"

#ifndef INPUT_GROUP_MAX
    #define INPUT_GROUP_MAX 16
#endif

typedef struct {
    Input_t *inputs[INPUT_GROUP_MAX];
    MatcherPatternGlobal_t *matchers[INPUT_GROUP_MAX];   // NULL si pas de matcher
    uint8_t count;
} InputGroup_t;

// Initialisation
bool input_group_create(InputGroup_t *group);

// Ajout d'un input ou d'un matcher
bool input_group_add_input(InputGroup_t *group, Input_t *input);
bool input_group_add_matcher(InputGroup_t *group, MatcherPatternGlobal_t *matcher);

// Mise à jour de tous les inputs (et leurs matchers)
void input_group_update_all(InputGroup_t *group, uint16_t ticks);

// Lecture groupée : retourne l'état stable de chaque input sur un bit
// bit 0 = inputs[0], bit 1 = inputs[1], etc.
uint32_t input_group_get_state(const InputGroup_t *group);

#endif