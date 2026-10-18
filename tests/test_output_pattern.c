#include "libpic/output_pattern.h"
#include "test_helpers.h"

static state_t g_pin = STATE_LOW;
static uint16_t g_last_pin = 0xFFFFu;

static void mock_write(uint16_t pin_id, state_t state)
{
    g_last_pin = pin_id;
    g_pin = state;
}

/* Séquence de 300 ms : 100 ms actif, 200 ms inactif. */
static const OutputPatternStep_t steps[] = {
    { true,  100 },
    { false, 200 }
};

static const OutputPatternSequence_t sequence = { steps, 2 };

static OutputPatternStepState_t buffer_data[2];
static OutputPatternStateBuffer_t buffer = { buffer_data, 2 };

static OutputPattern_t pattern;
static output_t out;

static void setup(bool abort_on_fail, TargetState_t state_on_fail, uint16_t cycles, time_ms_t now_ms)
{
    g_pin = STATE_LOW;
    g_last_pin = 0xFFFFu;

    output_init(&out, mock_write, 3, true, false);
    output_pattern_init(&pattern, &out);
    TEST_ASSERT(output_pattern_run(&pattern, &sequence, &buffer, cycles,
                                   abort_on_fail, state_on_fail, now_ms));
}

int main(void)
{
    OutputPatternEvent_t event;
    time_ms_t t;

    /* ---------- exécution nominale ---------- */
    setup(false, OUTPUT_OFF, 1, 0);
    TEST_ASSERT(output_is_active(&out));
    TEST_ASSERT(g_last_pin == 3);

    for (t = 10; t < 100; t += 10) {
        event = output_pattern_update(&pattern, t);
    }
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_RUNNING);

    /* Fin de la première étape : passage à l'étape inactive. */
    event = output_pattern_update(&pattern, 100);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_COMPLETED);
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(!output_is_active(&out));
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(!event.new_skipped);

    for (t = 110; t < 300; t += 10) {
        event = output_pattern_update(&pattern, t);
    }
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);

    /* Fin du dernier cycle. */
    event = output_pattern_update(&pattern, 300);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_COMPLETED);
    TEST_ASSERT(event.cycles_done == 1);
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(output_pattern_is_completed(&pattern));

    /* Un pattern terminé ne bouge plus. */
    event = output_pattern_update(&pattern, 400);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_COMPLETED);

    /* ---------- tick trop long, sans interruption ---------- */
    setup(false, OUTPUT_OFF, 1, 1000);
    event = output_pattern_update(&pattern, 1600);   /* 600 ms pour une étape de 100 ms */
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(!output_is_active(&out));

    /* Le défaut n'est signalé qu'une fois. */
    event = output_pattern_update(&pattern, 1610);
    TEST_ASSERT(!event.new_failed);

    /* ---------- tick trop long avec interruption sur échec ---------- */
    setup(true, OUTPUT_ON, 1, 2000);
    event = output_pattern_update(&pattern, 2600);
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(output_is_active(&out));            /* state_on_fail appliqué */
    TEST_ASSERT(g_last_pin == 3);

    /* Un pattern interrompu ne repart pas tout seul. */
    event = output_pattern_update(&pattern, 2700);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(!event.new_failed);

    /* ---------- pause avec un tick correct ---------- */
    setup(false, OUTPUT_OFF, 1, 3000);
    event = output_pattern_pause(&pattern, OUTPUT_OFF, 3050);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_PAUSED);
    TEST_ASSERT(!output_is_active(&out));

    event = output_pattern_update(&pattern, 3150);
    TEST_ASSERT(event.new_skipped);
    TEST_ASSERT(!event.new_failed);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_SKIPPED);
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(!output_is_active(&out));           /* la sortie reste figée */

    /* ---------- pause avec un tick trop long : échec comme en marche ---------- */
    setup(true, OUTPUT_ON, 1, 4000);
    (void)output_pattern_pause(&pattern, OUTPUT_OFF, 4000);
    event = output_pattern_update(&pattern, 4600);
    TEST_ASSERT(event.new_failed);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_FAILED);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_ABORDED);
    TEST_ASSERT(output_is_active(&out));

    /* ---------- arrêt puis reprise : le temps écoulé est restauré ---------- */
    setup(false, OUTPUT_OFF, 1, 5000);
    event = output_pattern_stop(&pattern, OUTPUT_OFF, 5040);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_STOPPED);
    TEST_ASSERT(!output_is_active(&out));

    event = output_pattern_update(&pattern, 6000);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_STOPPED);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_RUNNING);

    event = output_pattern_resume(&pattern, 7000);
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(output_is_active(&out));

    /* 40 ms consommés avant l'arrêt : 50 ms plus tard, l'étape court toujours. */
    event = output_pattern_update(&pattern, 7050);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_RUNNING);

    /* 40 + 60 = 100 ms : l'étape est validée. */
    event = output_pattern_update(&pattern, 7060);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_COMPLETED);

    /* ---------- enchaînement de deux cycles ---------- */
    setup(false, OUTPUT_OFF, 2, 8000);

    event = output_pattern_update(&pattern, 8100);   /* fin de l'étape 0 du 1er cycle */
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(!output_is_active(&out));

    event = output_pattern_update(&pattern, 8300);   /* fin du 1er cycle */
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_RUNNING);
    TEST_ASSERT(event.cycles_done == 1);
    TEST_ASSERT(buffer_data[0].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(output_is_active(&out));
    TEST_ASSERT(!output_pattern_is_completed(&pattern));

    event = output_pattern_update(&pattern, 8400);   /* fin de l'étape 0 du 2e cycle */
    TEST_ASSERT(buffer_data[1].state == OUTPUT_STEP_RUNNING);
    TEST_ASSERT(!output_is_active(&out));

    event = output_pattern_update(&pattern, 8600);   /* fin du 2e cycle */
    TEST_ASSERT(event.pattern_state == PATTERN_EXEC_COMPLETED);
    TEST_ASSERT(event.cycles_done == 2);
    TEST_ASSERT(output_pattern_is_completed(&pattern));

    printf("test_output_pattern: OK\n");
    return TEST_RESULT();
}
