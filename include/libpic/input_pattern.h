/**
 * @file input_pattern.h
 * @brief Détection de motifs (patterns) d'utilisation sur une entrée : appui court,
 *        appui long, double appui.
 */
#ifndef LIBPIC_INPUT_PATTERN_H
#define LIBPIC_INPUT_PATTERN_H

#include "commun.h"
#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Motif détecté lors de la dernière mise à jour, le cas échéant. */
typedef enum {
    INPUT_PATTERN_NONE = 0,   /**< Aucun motif détecté. */
    INPUT_PATTERN_SHORT_PRESS, /**< Appui court détecté. */
    INPUT_PATTERN_LONG_PRESS,  /**< Appui long détecté. */
    INPUT_PATTERN_DOUBLE_PRESS /**< Double appui détecté. */
} input_pattern_result_t;

/** Représente un détecteur de motifs d'appui sur une entrée. */
typedef struct {
    input_t *input;                    /**< Entrée surveillée. */
    time_ms_t long_press_threshold_ms; /**< Durée minimale, en ms, pour qualifier un appui long. */
    time_ms_t double_press_window_ms;  /**< Délai maximal, en ms, entre deux appuis courts pour un double appui. */
    time_ms_t press_start_time;        /**< Instant du début de l'appui en cours. */
    time_ms_t last_release_time;       /**< Instant du dernier relâchement (appui court). */
    bool pressed;                      /**< true si l'entrée est actuellement appuyée. */
    bool awaiting_second_press;        /**< true si un premier appui court attend un second appui. */
} input_pattern_t;

/**
 * Initialise un détecteur de motifs.
 *
 * @param pattern                Détecteur à initialiser.
 * @param input                  Entrée à surveiller (doit être mise à jour séparément via input_update).
 * @param long_press_threshold_ms Durée minimale d'appui pour qualifier un appui long.
 * @param double_press_window_ms  Délai maximal entre deux appuis courts pour un double appui.
 */
void input_pattern_init(input_pattern_t *pattern, input_t *input,
                         time_ms_t long_press_threshold_ms, time_ms_t double_press_window_ms);

/**
 * Met à jour le détecteur de motifs. Doit être appelée périodiquement, après
 * avoir mis à jour l'entrée surveillée (input_update).
 *
 * @param pattern Détecteur à mettre à jour.
 * @param now_ms  Instant courant en millisecondes.
 * @return Le motif détecté lors de cet appel, ou INPUT_PATTERN_NONE.
 */
input_pattern_result_t input_pattern_update(input_pattern_t *pattern, time_ms_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* LIBPIC_INPUT_PATTERN_H */
