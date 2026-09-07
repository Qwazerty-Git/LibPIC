    /*
    input.h

    Créé le: 19/08/2026
    Créé par: Qwazerty

    But : 
        Fournit une interface pour gérer des entrées binaires avec debounce.
        Chaque Input_t représente UNE entrée logique, mappée sur un GPIO physique.
        Le module gère :
            - Le debounce (élimination des rebonds mécaniques)
            - La détection de front (montant, descendant)
            - La lecture de l'état stable

    Fonctionnement :
        - L'utilisateur crée un Input_t avec input_create().
        - Il appelle input_update() régulièrement (ex: toutes les 1ms) avec les ticks écoulés.
        - Le module filtre les changements (debounce) et retourne les fronts détectés.
        - Un callback optionnel peut être enregistré via input_set_callback().

    Exemple d'utilisation :
        #include "input.h"

        Input_t bouton;
        input_create(&bouton, (volatile uint8_t*)&PORTB, 0x10, false, 20, 50);

        // Dans la boucle principale (appelée toutes les 1ms) :
        InputEvent_t event = input_update(&bouton, 1);
        if (event.rising) {
            // Le bouton vient d'être pressé
        }
        if (event.falling) {
            // Le bouton vient d'être relâché
        }
    */

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
//#include <stddef.h>
#include <stdbool.h>
#include "commun.h"

// Événement retourné par input_update
typedef struct {
    bool rising;    // Front montant détecté (appui)
    bool falling;   // Front descendant détecté (relâchement)
    bool reliable;  // true si ticks <= debounce_max (le debounce a un sens)
} InputEvent_t;

// Définition du callback (optionnel)
typedef void (*InputReceiver)(void *context, const InputEvent_t event);

typedef struct {
    // Configuration publique
    GPIO_t mapping;
    bool active_high;
    bool enabled;
    uint16_t debounce_rising_ticks;   // Ticks pour valider un front montant
    uint16_t debounce_falling_ticks;  // Ticks pour valider un front descendant

    // Callback optionnel
    InputReceiver receiver;
    void *receiver_context;

    // Variables internes (à ne pas modifier)
    bool state_raw_debounced;          // État validé après debounce
    bool state_raw;             // Dernière lecture brute
    uint16_t counter;           // Compteur de debounce
} Input_t;

// Initialisation
bool input_create(Input_t *input, volatile uint8_t *port, uint8_t mask, bool active_high, uint16_t debounce_rising_ticks, uint16_t debounce_falling_ticks);

// Enregistrement d'un callback (optionnel)
void input_set_callback(Input_t *input, InputReceiver receiver, void *context);

// Mise à jour (à appeler périodiquement)
InputEvent_t input_update(Input_t *input, uint16_t ticks);

// Activation/désactivation
void input_enable(Input_t *input, bool enable);

// Lectures
bool input_is_physically_high(const Input_t *input);  // Lecture directe du GPIO (brut)
bool input_is_active(const Input_t *input);            // État validé (debounce)
bool input_state_stable(const Input_t *input);  // État validé par le debounce (même que input_is_active)

#endif