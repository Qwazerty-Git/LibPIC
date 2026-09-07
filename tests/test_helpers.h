#ifndef LIBPIC_TEST_HELPERS_H
#define LIBPIC_TEST_HELPERS_H

#include <stdio.h>

/*
 * NOTE: this header intentionally avoids <stdlib.h> (and thus exit()/abort()),
 * since on some hosts it transitively declares the POSIX `timer_t` type,
 * which would clash with LibPIC's own `timer_t` type used in the tests.
 */
static int libpic_test_failures = 0;

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
            libpic_test_failures++; \
        } \
    } while (0)

#define TEST_RESULT() (libpic_test_failures == 0 ? 0 : 1)

#endif /* LIBPIC_TEST_HELPERS_H */
