/*
output.h

Créé le: 19/08/2026
Créé par: Qwazerty

But :
    Fournit une interface pour gérer des sorties binaires individuelles.
    Chaque Output_t représente UNE sortie logique, mappée sur un GPIO physique.
    Le module gère la logique active_high (niveau actif haut ou bas).

Fonctionnement :
    - L'utilisateur crée un Output_t avec output_create().
    - Il modifie l'état logique avec output_on(), output_off(), etc.
    - Le module calcule le niveau physique en fonction de active_high.
    - L'écriture GPIO peut être immédiate (fonction output_update_gpio()).

Exemple d'utilisation :

    #include "output.h"

    // Déclaration statique (pas de malloc)
    Output_t led_verte;

    // Initialisation : PORTB bit 0, actif à l'état HAUT, éteinte au départ
    output_create(&led_verte, (volatile uint8_t*)&PORTB, 0x01, true, false);

    // Dans la boucle principale :
    output_on(&led_verte);   // Allume la LED
    output_off(&led_verte);  // Éteint la LED
    output_toggle(&led_verte);  // Inverse l'état

Méthodes exposées :

    - output_create : 
        Initialise une sortie avec son mapping GPIO, sa polarité et son état par défaut.

    - output_on : 
        Met l'état logique à 1 (haut). Le GPIO est mis à jour en fonction de active_high.

    - output_off : 
        Met l'état logique à 0 (bas). Le GPIO est mis à jour en fonction de active_high.

    - output_toggle : 
        Inverse l'état logique actuel. Retourne le nouvel état.

    - output_set_state : 
        Définit un état logique spécifique (true ou false).

    - output_enable : 
        Active/désactive la sortie. Si désactivée, l'état peut être modifié
        mais le GPIO n'est pas écrit.

    - output_update_gpio : 
        Force l'écriture du GPIO en fonction de l'état et de active_high.
        Utile si on a modifié l'état sans auto-update.

    - output_is_on : 
        Retourne l'état LOGIQUE actuel (true si allumé).

    - output_is_physically_high : 
        Retourne l'état PHYSIQUE du GPIO (true si le niveau est haut).
        Diffère de output_is_on si active_high = false.

Note :
    - Les variables GPIO doivent être déclarées volatile (ex: *(volatile uint8_t*)&PORTB).
    - L'écriture GPIO est immédiate à chaque changement d'état.
*/

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "commun.h"

typedef struct {
    GPIO_t mapping;
    bool active_high;
    bool state;
    bool enabled;
} Output_t;

// Initialisation
bool output_create(Output_t *output, volatile uint8_t *port, uint8_t mask, bool active_high, bool default_state);

// Actions sur le maillon
void output_on(Output_t *output);
void output_off(Output_t *output);
bool output_toggle(Output_t *output);
void output_set_state(Output_t *output, bool state);
void output_enable(Output_t *output, bool enable);

// Écriture GPIO
void output_update_gpio(Output_t *output);

// Lectures
bool output_is_on(const Output_t *output);
bool output_is_physically_high(const Output_t *output);


#endif