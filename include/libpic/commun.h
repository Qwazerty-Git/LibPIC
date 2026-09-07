/*
    commun.h
    Créé le: 22-08-2026
    Crée par: Qwazerty

 */

#ifndef COMMUN_H
#define COMMUN_H

#include <stdint.h>

// Définis ici pour garantir la même taille de InputGroup_t/OutputGroup_t
// dans toutes les unités de compilation (sinon conflit de linkage sous XC8).
#ifndef INPUT_GROUP_MAX
    #define INPUT_GROUP_MAX 4
#endif
#ifndef OUTPUT_GROUP_MAX
    #define OUTPUT_GROUP_MAX 4
#endif

typedef struct
{
    volatile uint8_t *port; 		// Registre de lecture/écriture (PORTB, PORTC, ...)
    uint8_t mask;           		// Masque identique pour PORT et TRIS
} GPIO_t;



#endif 