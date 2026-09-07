/**
 * @file timer.h
 * @brief Gestion de temporisations logicielles (non bloquantes).
 */
#ifndef LIBPIC_TIMER_H
#define LIBPIC_TIMER_H

#include "commun.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Représente une temporisation logicielle non bloquante. */
typedef struct {
    time_ms_t duration_ms;    /**< Durée de la temporisation, en millisecondes. */
    time_ms_t start_time_ms;  /**< Instant de démarrage de la temporisation. */
    bool running;             /**< true si la temporisation est démarrée. */
    bool auto_reload;         /**< true si la temporisation redémarre automatiquement à expiration. */
} timer_t;

/**
 * Initialise une temporisation.
 *
 * @param timer       Temporisation à initialiser.
 * @param duration_ms Durée de la temporisation, en millisecondes.
 * @param auto_reload true pour un redémarrage automatique à chaque expiration.
 */
void timer_init(timer_t *timer, time_ms_t duration_ms, bool auto_reload);

/** Démarre (ou redémarre) la temporisation à partir de l'instant courant. */
void timer_start(timer_t *timer, time_ms_t now_ms);

/** Arrête la temporisation. */
void timer_stop(timer_t *timer);

/**
 * Met à jour la temporisation. Doit être appelée périodiquement.
 *
 * @param timer  Temporisation à mettre à jour.
 * @param now_ms Instant courant en millisecondes.
 * @return true si la temporisation vient d'expirer lors de cet appel.
 *
 * @note Pour une temporisation à redémarrage automatique (auto_reload),
 *       si plusieurs périodes ont été manquées entre deux appels, l'instant
 *       de départ est resynchronisé en une seule fois (les périodes
 *       manquées ne sont pas rejouées individuellement).
 */
bool timer_update(timer_t *timer, time_ms_t now_ms);

/** Retourne true si la temporisation est en cours d'exécution. */
bool timer_is_running(const timer_t *timer);

/** Retourne la durée écoulée depuis le démarrage de la temporisation. */
time_ms_t timer_elapsed(const timer_t *timer, time_ms_t now_ms);

/** Retourne la durée restante avant expiration de la temporisation. */
time_ms_t timer_remaining(const timer_t *timer, time_ms_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_TIMER_H */
