/*
timer.h

Créé le: 22/08/2026
Créé par: Qwazerty

But : 
    Fournit une interface pour gérer des timers basés sur des cycles.
    Le timer compte des ticks, et lorsqu'il atteint sa durée, il génère un évènement.
    Il peut être configuré pour un nombre limité de cycles (puis pause automatique),
    ou en mode infini (nbr_cycles = 0).

Fonctionnement :
    Le timer est mis à jour via timer_update(), qui reçoit le nombre de ticks écoulés.
    Quand le compteur atteint la durée configurée, un évènement est généré et retourné.
    Le timer peut être mis en pause et repris.
    Un callback optionnel peut être enregistré pour recevoir les évènements.

Méthodes exposées :

    - timer_create : 
        Initialise un timer à zéro. Doit être appelé avant toute autre fonction.

    - timer_init : 
        Configure la durée du timer (en ticks) et le nombre de cycles.
        nbr_cycles = 0 signifie infini.
        Exemple : timer_init(&timer, 1000, 3) → timer qui déclenche toutes les 1000 ticks, 3 fois.

    - timer_set_receiver : 
        Enregistre un callback qui sera appelé quand un évènement est généré.
        Le contexte est passé au callback.

    - timer_update : 
        Met à jour le timer avec le nombre de ticks écoulés.
        Retourne un TimerEvent_t qui indique si un évènement s'est produit.
        L'utilisateur appelle cette fonction périodiquement (ex: dans une boucle principale).

    - timer_set_duration : 
        Modifie la durée du timer en vol.
        Le compteur est remis à zéro.

    - timer_reset : 
        Remet à zéro le compteur, l'itération et la pause.

    - timer_pause : 
        Met le timer en pause. Les ticks reçus sont ignorés.

    - timer_resume : 
        Reprend le timer après une pause.
        Si le timer avait terminé ses cycles, il repart pour un nouveau cycle complet.

    - timer_is_paused : 
        Indique si le timer est en pause.

    - timer_cycle_over : 
        Indique si le nombre de cycles configuré a été atteint.

Note : 
    Les fonctions de configuration vérifient les paramètres et retournent false en cas d'erreur.
    Le timer est conçu pour être utilisé dans un environnement temps réel (embarqué).
*/

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

// Structure de retour de la fonction timer_update
typedef struct
{
    bool event;              // Indique si un évènement s'est produit
    uint32_t iteration;      // Numéro de l'itération actuelle (0, 1, 2, ...)
    bool iteration_overflow; // Indique si le compteur d'itération a dépassé uint32_t
    bool cycle_over;         // Indique si le nombre de cycles configuré est atteint
} TimerEvent_t;

// Définition du callback
typedef void (*TimerReceiver)(void *context, const TimerEvent_t timer_event);

typedef struct
{
    TimerReceiver receiver;          // Callback optionnel
    void *receiver_context;          // Contexte passé au callback

    bool paused;                     // État de pause

    uint16_t nbr_cycles;             // Nombre de cycles avant pause (0 = infini)
    uint16_t duration;               // Durée du timer en ticks
    uint32_t counter;                // Compteur actuel
    uint32_t iteration;              // Itération actuelle
} Timer_t;

// Fonctions de gestion
bool timer_create(Timer_t *timer);
bool timer_init(Timer_t *timer, uint16_t duration, uint16_t cycle);
void timer_set_receiver(Timer_t *timer, TimerReceiver receiver, void *context);

TimerEvent_t timer_update(Timer_t *timer, uint16_t ticks);
bool timer_set_duration(Timer_t *timer, uint16_t duration);

bool timer_is_paused(const Timer_t *timer);
bool timer_cycle_over(const Timer_t *timer);

void timer_reset(Timer_t *timer);
void timer_pause(Timer_t *timer);
TimerEvent_t timer_resume(Timer_t *timer);

#endif