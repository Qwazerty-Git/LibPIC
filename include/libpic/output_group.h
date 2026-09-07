/*
output_group.h

Créé le: 19/08/2026
Créé par: Qwazerty

But :
    Fournit une interface pour gérer un GROUPE de sorties binaires.
    Permet de traiter plusieurs Output_t ensemble (activer, recevoir 32 bits, etc.).
    La taille maximale est définie par OUTPUT_GROUP_MAX (16 par défaut).

Fonctionnement :
    - L'utilisateur crée un OutputGroup_t avec output_group_create().
    - Il ajoute des Output_t avec output_group_add().
    - Le groupe donne accès aux maillons par index (position 0, 1, 2, ...).
    - Les fonctions "all" / "mask" / "receive" agissent sur tout le groupe.

Exemple d'utilisation :

    #include "output.h"
    #include "output_group.h"

    // Déclaration des sorties individuelles
    Output_t led_rouge, led_verte, led_bleue;

    // Déclaration du groupe
    OutputGroup_t leds;

    void init_leds(void) {
        // Création des sorties (PORTB bits 0, 1, 2)
        output_create(&led_rouge, (volatile uint8_t*)&PORTB, 0x01, true, false);
        output_create(&led_verte, (volatile uint8_t*)&PORTB, 0x02, true, false);
        output_create(&led_bleue, (volatile uint8_t*)&PORTB, 0x04, true, false);

        // Création du groupe et ajout des sorties
        output_group_create(&leds);
        output_group_add(&leds, &led_rouge);
        output_group_add(&leds, &led_verte);
        output_group_add(&leds, &led_bleue);
    }

    // Exemple 1 : allumer la LED verte (index 1)
    Output_t *led = output_group_get(&leds, 1);
    output_on(led);

    // Exemple 2 : recevoir 0b101 → rouge allumée, verte éteinte, bleue allumée
    output_group_receive(&leds, 0b101);

    // Exemple 3 : activer sélectivement les sorties 0 et 2 (masque 0b101)
    output_group_enable_mask(&leds, 0b101);

    // Exemple 4 : éteindre tout le groupe
    output_group_set_state_all(&leds, false);

Méthodes exposées :

    - output_group_create : 
        Initialise un groupe vide (tous les pointeurs à NULL, count = 0).

    - output_group_add : 
        Ajoute un Output_t au groupe. Retourne son index (0-15) ou -1 si erreur.

    - output_group_get : 
        Retourne le pointeur vers l'Output_t à l'index donné (ou NULL si index invalide).

    - output_group_update_gpio_all : 
        Écrit le GPIO de toutes les sorties du groupe (selon leur état).

    - output_group_receive : 
        Reçoit une valeur 32 bits. Chaque bit i est appliqué à la sortie i (0-15).
        Exemple : value = 0b101 → sortie 0 = 1, sortie 1 = 0, sortie 2 = 1.

    - output_group_enable_all : 
        Active ou désactive toutes les sorties du groupe.

    - output_group_enable_mask : 
        Active/désactive sélectivement les sorties selon un masque 32 bits.
        Exemple : mask = 0b1010 → sorties 1 et 3 activées, autres désactivées.

    - output_group_set_state_all : 
        Définit le même état (true/false) pour toutes les sorties du groupe.

Note :
    - OUTPUT_GROUP_MAX = 16 (modifiable).
    - Utilisez les fonctions output_* du module output.h pour agir sur un maillon individuel.
    - Utilisez les fonctions output_group_* pour agir sur tout le groupe.
*/

#ifndef OUTPUT_GROUP_H
#define OUTPUT_GROUP_H

#include <stdint.h>
#include <stdbool.h>
#include "output.h"

#ifndef OUTPUT_GROUP_MAX
    #define OUTPUT_GROUP_MAX 16
#endif

typedef struct {
    Output_t *outputs[OUTPUT_GROUP_MAX];  // Pointeurs vers les maillons
    uint8_t count;                        // Nombre de maillons enregistrés
} OutputGroup_t;

// Initialisation
bool output_group_create(OutputGroup_t *group);

// Ajout d'un maillon au groupe (retourne l'index ou -1 si plein)
int8_t output_group_add(OutputGroup_t *group, Output_t *output);

// Accès à un maillon
Output_t *output_group_get(const OutputGroup_t *group, uint8_t index);

// Actions sur tous les maillons
void output_group_update_gpio_all(OutputGroup_t *group);
void output_group_receive(OutputGroup_t *group, uint32_t value);
void output_group_enable_all(OutputGroup_t *group, bool enable);
void output_group_enable_mask(OutputGroup_t *group, uint32_t mask);
void output_group_set_state_all(OutputGroup_t *group, bool state);

#endif