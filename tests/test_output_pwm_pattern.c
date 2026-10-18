#include "libpic/output_pwm_pattern.h"
#include "test_helpers.h"

static uint8_t g_duty = 0xFFu;
static uint16_t g_last_pin = 0xFFFFu;

static void mock_write_pwm(uint16_t pin_id, uint8_t duty)
{
    g_last_pin = pin_id;
    g_duty = duty;
}

/* Séquence de 300 ms : 100 ms à 20 %, puis 200 ms à 100 %. */
static const OutputPwmPatternStep_t steps[] = {
    { 20,  100 },
    { 100, 200 }
};

static const OutputPwmPatternSequence_t sequence = { steps, 2 };

static OutputPatternStepState_t states[2];

static OutputPwmPattern_t pattern;
static output_pwm_t pwm;

static void setup(bool abort_on_fail, uint8_t state_on_fail, uint8_t cycles, time_ms_t now_ms)
{
    g_duty = 0xFFu;
    g_last_pin = 0xFFFFu;

    output_pwm_init(&pwm, mock_write_pwm, 4, 0, true);
    output_pwm_pattern_init(&pattern, &pwm);
    TEST_ASSERT(output_pwm_pattern_run(&pattern, &sequence, states,
                                       (uint8_t)(sizeof(states) / sizeof(states[0])),
                                       cycles, abort_on_fail, state_on_fail, now_ms));
}

int main(void)
{
    OutputPatternEvent_t event;
    time_ms_t t;

    /* ---------- exécution nominale ---------- */
    setup(false, 0, 1, 0);
    TEST_ASSERT(g_duty == 20);
    TEST_ASSERT(g_last_pin == 4);

    for (t = 10; t < 100; t += 10) {
        event = output_pwm_pattern_update(&pattern, t);
    }
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 20);          /* pas de réécriture entre deux étapes */

    /* Fin de la première étape : passage à l'étape pleine. */
    event = output_pwm_pattern_update(&pattern, 100);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_COMPLETED);
    TEST_ASSERT(states[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 100);
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(!event.new_skipped);

    for (t = 110; t < 300; t += 10) {
        event = output_pwm_pattern_update(&pattern, t);
    }
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);

    /* Fin du dernier cycle. */
    event = output_pwm_pattern_update(&pattern, 300);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(states[1].state == OUTPUT_STEP_COMPLETED);
    TEST_ASSERT(event.cycles_done == 1);
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(output_pwm_pattern_is_completed(&pattern));

    /* Un pattern terminé ne bouge plus. */
    event = output_pwm_pattern_update(&pattern, 400);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(states[1].state == OUTPUT_STEP_COMPLETED);

    /* ---------- tick trop long, sans interruption ---------- */
    setup(false, 0, 1, 1000);
    event = output_pwm_pattern_update(&pattern, 1600);   /* 600 ms pour une étape de 100 ms */
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(states[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 100);

    /* Le défaut n'est signalé qu'une fois. */
    event = output_pwm_pattern_update(&pattern, 1610);
    TEST_ASSERT(!event.new_failed);

    /* ---------- tick trop long avec interruption sur échec ---------- */
    setup(true, 0, 1, 2000);
    event = output_pwm_pattern_update(&pattern, 2600);
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(g_duty == 0);                       /* state_on_fail appliqué */
    TEST_ASSERT(g_last_pin == 4);

    /* Un pattern interrompu ne repart pas tout seul. */
    event = output_pwm_pattern_update(&pattern, 2700);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(!event.new_failed);

    /* ---------- pause avec un tick correct : la sortie est figée ---------- */
    setup(false, 0, 1, 3000);
    event = output_pwm_pattern_pause(&pattern, 0, 3050);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_PAUSED);
    TEST_ASSERT(g_duty == 0);

    event = output_pwm_pattern_update(&pattern, 3150);
    TEST_ASSERT(event.new_skipped);
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_SKIPPED);
    TEST_ASSERT(states[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 0);                       /* la sortie reste figée */

    /* ---------- pause avec OUTPUT_PWM_DUTY_KEEP : la sortie n'est pas touchée ---------- */
    setup(false, 0, 1, 3500);
    event = output_pwm_pattern_pause(&pattern, OUTPUT_PWM_DUTY_KEEP, 3550);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_PAUSED);
    TEST_ASSERT(g_duty == 20);                      /* rapport cyclique de l'étape courante */

    /* ---------- pause avec un tick trop long : échec comme en marche ---------- */
    setup(true, 100, 1, 4000);
    (void)output_pwm_pattern_pause(&pattern, 0, 4000);
    event = output_pwm_pattern_update(&pattern, 4600);
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(g_duty == 100);

    /* ---------- arrêt puis reprise : le temps écoulé est restauré ---------- */
    setup(false, 0, 1, 5000);
    event = output_pwm_pattern_stop(&pattern, 0, 5040);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_STOPPED);
    TEST_ASSERT(g_duty == 0);

    event = output_pwm_pattern_update(&pattern, 6000);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_STOPPED);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_RUNNING);

    event = output_pwm_pattern_resume(&pattern, 7000);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(g_duty == 20);                      /* le rapport cyclique de l'étape est réappliqué */

    /* 40 ms consommés avant l'arrêt : 50 ms plus tard, l'étape court toujours. */
    event = output_pwm_pattern_update(&pattern, 7050);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_RUNNING);

    /* 40 + 60 = 100 ms : l'étape est validée. */
    event = output_pwm_pattern_update(&pattern, 7060);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_COMPLETED);

    /* ---------- enchaînement de deux cycles ---------- */
    setup(false, 0, 2, 8000);

    event = output_pwm_pattern_update(&pattern, 8100);   /* fin de l'étape 0 du 1er cycle */
    TEST_ASSERT(states[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 100);

    event = output_pwm_pattern_update(&pattern, 8300);   /* fin du 1er cycle */
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(event.cycles_done == 1);
    TEST_ASSERT(states[0].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 20);
    TEST_ASSERT(!output_pwm_pattern_is_completed(&pattern));

    event = output_pwm_pattern_update(&pattern, 8400);   /* fin de l'étape 0 du 2e cycle */
    TEST_ASSERT(states[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(g_duty == 100);

    event = output_pwm_pattern_update(&pattern, 8600);   /* fin du 2e cycle */
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(event.cycles_done == 2);
    TEST_ASSERT(output_pwm_pattern_is_completed(&pattern));

    printf("test_output_pwm_pattern: OK\n");
    return TEST_RESULT();
}
