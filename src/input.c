#include "input.h"
#include <stddef.h>

// Lecture de l'état physique du GPIO
static bool input_read_physical(const Input_t *input)
{
    if (input == NULL || input->mapping.port == NULL) return false;
    return (*input->mapping.port & input->mapping.mask) != 0;
}

bool input_create(Input_t *input, volatile uint8_t *port, uint8_t mask, bool active_high, uint16_t debounce_rising_ticks, uint16_t debounce_falling_ticks)
{
    if (input == NULL || port == NULL) return false;

    input->mapping.port = port;
    input->mapping.mask = mask;
    input->active_high = active_high;
    input->enabled = true;
    input->debounce_rising_ticks = debounce_rising_ticks;
    input->debounce_falling_ticks = debounce_falling_ticks;

    input->receiver = NULL;
    input->receiver_context = NULL;

    // État initial
    input->state_raw = input_read_physical(input);
    input->state_raw_debounced = input->state_raw;
    input->counter = 0;

    return true;
}

void input_set_callback(Input_t *input, InputReceiver receiver, void *context)
{
    if (input == NULL || input->receiver == NULL || receiver == NULL || context == NULL ) return;
    input->receiver = receiver;
    input->receiver_context = context;
}

InputEvent_t input_update(Input_t *input, uint16_t ticks)
{
    InputEvent_t event = {0};

    if (input == NULL || !input->enabled) return event;

    // Lire l'état physique actuel
    input->state_raw = input_read_physical(input);

    // Appliquer active_high pour obtenir l'état logique
    bool state_logical = (input->state_raw == input->active_high);

    // La fiabilité est bonne si on n'a pas "sauté" le debounce
    uint16_t max_debounce = (input->debounce_rising_ticks > input->debounce_falling_ticks)
                            ? input->debounce_rising_ticks
                                                : input->debounce_falling_ticks;

    bool reliable = (ticks <= max_debounce);

    // Debounce : attendre que l'état soit stable pendant debounce_ticks
    if (state_logical != input->state_raw_debounced) {
        input->counter += ticks;

        // Le temps de debounce dépend du sens du changement (rising ou falling logique)
        uint16_t required_ticks = state_logical ? input->debounce_rising_ticks
                                                : input->debounce_falling_ticks;

        if (input->counter >= required_ticks) {
            // Le changement est validé
            input->state_raw_debounced = state_logical;

            // Détecter le front
            if (state_logical) {
                event.rising = true;   // Front montant logique (appui)
            } else {
                event.falling = true;  // Front descendant logique (relâchement)
            }

            event.reliable = reliable;

            // Appeler le callback si enregistré (uniquement si événement)
            if (input->receiver != NULL && input->receiver_context !=NULL) {
                input->receiver(input->receiver_context, event);
            }

            // Remettre le compteur à zéro pour la prochaine transition
            input->counter = 0;
        }
    } else {
        // État stable, pas de comptage
        input->counter = 0;
    }

    return event;
}

void input_enable(Input_t *input, bool enable)
{
    if (input == NULL) return;
    input->enabled = enable;

    // Reset de l'état pour éviter de faux événements après réactivation
    if (enable) {
        input->state_raw = input_read_physical(input);
        input->state_raw_debounced = input->state_raw;
        input->counter = 0;
    }
}

bool input_is_physically_high(const Input_t *input)
{
    if (input == NULL || input->mapping.port == NULL) return false;
    return (*input->mapping.port & input->mapping.mask) != 0;  // ← lecture directe
}

bool input_is_active(const Input_t *input)
{
    if (input == NULL) return false;
    return input->state_raw_debounced;  // ← état validé par le debounce
}

bool input_state_stable(const Input_t *input)
{
    if (input == NULL) return false;
    return input->state_raw_debounced == input->active_high;  // État stable logique (après debounce)
}