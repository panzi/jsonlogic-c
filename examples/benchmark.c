// for clock_gettime()
#define _GNU_SOURCE 1

#include "jsonlogic.h"
#include "jsonlogic_extras.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>

void usage(int argc, char *argv[]) {
    const char *progname = argc > 0 ? argv[0] : "benchmark";
    fprintf(stderr, "usage: %s <repeat-count> <logic> <data>\n", progname);
}

#ifdef JSONLOGIC_WINDOWS
    #include <windows.h>
    #define DEVNULL "NUL"
    #define JSONLOGIC_CLOCK ULONGLONG
#else
    #define DEVNULL "/dev/null"
    #define JSONLOGIC_CLOCK struct timespec
#endif

#ifndef JSONLOGIC_WINDOWS
int64_t timedelta(const struct timespec *t1, const struct timespec *t2) {
    assert((uint64_t)t1->tv_sec <= INT64_MAX / 1000000);
    assert((uint64_t)t2->tv_sec <= INT64_MAX / 1000000);
    assert((uint64_t)t1->tv_sec * 1000000 < INT64_MAX - (uint64_t)t1->tv_nsec / 1000);
    assert((uint64_t)t2->tv_sec * 1000000 < INT64_MAX - (uint64_t)t2->tv_nsec / 1000);

    int64_t nsec1 = (uint64_t)t1->tv_sec * 1000000 + (uint64_t)t1->tv_nsec / 1000;
    int64_t nsec2 = (uint64_t)t2->tv_sec * 1000000 + (uint64_t)t2->tv_nsec / 1000;
    assert(nsec2 >= nsec1);

    return nsec2 - nsec1;
}
#endif

int compare(const void *ptr1, const void *ptr2) {
    int64_t t1 = *(int64_t*)ptr1;
    int64_t t2 = *(int64_t*)ptr2;

    return t1 > t2 ? 1 : t1 < t2 ? -1 : 0;
}

void stats(int64_t *times, size_t count, int64_t *minptr, int64_t *maxptr, int64_t *sumptr, int64_t *avgptr, int64_t *medianptr) {
    qsort(times, count, sizeof(int64_t), &compare);
    int64_t sum = 0;
    int64_t min = INT64_MAX;
    int64_t max = INT64_MIN;
    for (size_t index = 0; index < count; ++ index) {
        int64_t value = times[index];
        sum += value;
        if (value > max) {
            max = value;
        }
        if (value < min) {
            min = value;
        }
    }

    if (count % 2 == 0) {
        *medianptr = (times[count / 2 - 1] + times[count / 2]) / 2;
    } else {
        *medianptr = times[count / 2];
    }

    *sumptr = sum;
    *minptr = min;
    *maxptr = max;
    *avgptr = sum / count;
}

void print_stats(int64_t *times, size_t count, const char *name) {
    int64_t min = 0;
    int64_t max = 0;
    int64_t sum = 0;
    int64_t avg = 0;
    int64_t median = 0;

    stats(times, count, &min, &max, &sum, &avg, &median);

    printf("%-5s %10.3f %10.3f %10.3f %10.3f %10.3f\n",
        name,
        (double)min    / 1000.0,
        (double)max    / 1000.0,
        (double)avg    / 1000.0,
        (double)median / 1000.0,
        (double)sum    / 1000.0
    );
}

int main(int argc, char *argv[]) {
    int status = 0;
    int64_t *parse_times = NULL;
    int64_t *apply_times = NULL;
    int64_t *print_times = NULL;
    int64_t *free_times  = NULL;
    int64_t *sum_times   = NULL;
    FILE *devnull = NULL;
    JsonLogic_Handle logic  = JsonLogic_Null;
    JsonLogic_Handle data   = JsonLogic_Null;
    JsonLogic_Handle result = JsonLogic_Null;

    if (argc != 4) {
        usage(argc, argv);
        goto error;
    }

    const char *str_repeat_count = argv[1];
    const char *str_logic        = argv[2];
    const char *str_data         = argv[3];

    char *endptr = NULL;
    const unsigned long long ull_count = strtoull(str_repeat_count, &endptr, 10);
    if (!*str_repeat_count || *endptr || (sizeof(unsigned long long) > sizeof(size_t) && ull_count > (unsigned long long)SIZE_MAX) || ull_count == 0) {
        fprintf(stderr, "*** error: parsing repeat-count '%s': %s", str_repeat_count, strerror(errno));
        usage(argc, argv);
        return 1;
    }
    const size_t count = (size_t) ull_count;

    parse_times = calloc(count, sizeof(int64_t));
    if (parse_times == NULL) {
        perror("*** error: allocating memory");
        goto error;
    }

    apply_times = calloc(count, sizeof(int64_t));
    if (apply_times == NULL) {
        perror("*** error: allocating memory");
        goto error;
    }

    print_times = calloc(count, sizeof(int64_t));
    if (print_times == NULL) {
        perror("*** error: allocating memory");
        goto error;
    }

    free_times = calloc(count, sizeof(int64_t));
    if (free_times == NULL) {
        perror("*** error: allocating memory");
        goto error;
    }

    sum_times = calloc(count, sizeof(int64_t));
    if (sum_times == NULL) {
        perror("*** error: allocating memory");
        goto error;
    }

    devnull = fopen(DEVNULL, "wb");
    if (devnull == NULL) {
        perror("*** error: opening " DEVNULL);
        goto error;
    }

    // parse it once to test if it is syntactically correct
    JsonLogic_LineInfo info = JSONLOGIC_LINEINFO_INIT;
    logic = jsonlogic_parse(str_logic, &info);
    JsonLogic_Error error = jsonlogic_get_error(logic);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error(stderr, str_logic, error, info);
        goto error;
    }

    data = jsonlogic_parse(str_data, &info);
    error = jsonlogic_get_error(data);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error(stderr, str_data, error, info);
        goto error;
    }

    result = jsonlogic_apply_custom(logic, data, &JsonLogic_Extras);
    error = jsonlogic_get_error(result);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        fprintf(stderr, "*** error: evaluating logic: %s\n", jsonlogic_get_error_message(error));
        goto error;
    }

    jsonlogic_decref(logic);
    logic = JsonLogic_Null;

    jsonlogic_decref(data);
    data = JsonLogic_Null;

    jsonlogic_decref(result);
    result = JsonLogic_Null;

    for (size_t index = 0; index < (size_t)count; ++ index) {
        JSONLOGIC_CLOCK start;
        JSONLOGIC_CLOCK parse_done;
        JSONLOGIC_CLOCK apply_done;
        JSONLOGIC_CLOCK print_done;
        JSONLOGIC_CLOCK free_done;

#ifdef JSONLOGIC_WINDOWS
    #define GET_CLOCK(CLOCK) CLOCK = GetTickCount64();
    #define CLOCK_DELTA(C1, C2) (((C2) - (C1)) * 1000)
#else
    #define GET_CLOCK(CLOCK)                                   \
        if (clock_gettime(CLOCK_MONOTONIC, &(CLOCK)) != 0) {   \
            perror("*** error: getting monotonic time");       \
            goto error;                                        \
        }
    #define CLOCK_DELTA(C1, C2) timedelta(&(C1), &(C2))
#endif

        GET_CLOCK(start);

        logic = jsonlogic_parse(str_logic, NULL);
        data  = jsonlogic_parse(str_data, NULL);

        GET_CLOCK(parse_done);

        result = jsonlogic_apply_custom(logic, data, &JsonLogic_Extras);

        GET_CLOCK(apply_done);

        jsonlogic_println(devnull, result);

        GET_CLOCK(print_done);

        jsonlogic_decref(logic);
        jsonlogic_decref(data);
        jsonlogic_decref(result);

        GET_CLOCK(free_done);

        logic = JsonLogic_Null;
        data  = JsonLogic_Null;

        int64_t parse_time = CLOCK_DELTA(start,      parse_done);
        int64_t apply_time = CLOCK_DELTA(parse_done, apply_done);
        int64_t print_time = CLOCK_DELTA(apply_done, print_done);
        int64_t free_time  = CLOCK_DELTA(print_done, free_done);
        int64_t sum_time   = CLOCK_DELTA(start,      free_done);

        parse_times[index] = parse_time;
        apply_times[index] = apply_time;
        print_times[index] = print_time;
        free_times [index] = free_time;
        sum_times  [index] = sum_time;
    }

    printf("             min        max        avg     median        sum\n");
    print_stats(parse_times, count, "parse");
    print_stats(apply_times, count, "apply");
    print_stats(print_times, count, "print");
    print_stats(free_times,  count, "free");
    print_stats(sum_times,   count, "sum");

    goto cleanup;

error:
    status = 1;

cleanup:
    free(parse_times);
    free(apply_times);
    free(print_times);
    free(free_times);
    free(sum_times);

    if (devnull != NULL) {
        fclose(devnull);
    }

    jsonlogic_decref(logic);
    jsonlogic_decref(data);
    jsonlogic_decref(result);

    return status;
}
