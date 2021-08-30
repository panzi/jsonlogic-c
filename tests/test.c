// for fileno()
#define _POSIX_C_SOURCE 1

#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

int main(int argc, char *argv[]) {

    if (argc != 2) {
        const char *progname = argc > 0 ? argv[0] : "test";
        fprintf(stderr, "usage: %s <path/to/tests.json>\n", progname);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return 1;
    }

    struct stat stbuf;
    if (fstat(fileno(fp), &stbuf) != 0) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    char *tests_json = malloc(stbuf.st_size);
    if (tests_json == NULL) {
        fclose(fp);
        perror(filename);
        return 1;
    }

    if (fread(tests_json, stbuf.st_size, 1, fp) != 1) {
        free(tests_json);
        fclose(fp);
        perror(filename);
        return 1;
    }

    fclose(fp);

    JsonLogic_LineInfo info = JSONLOGIC_LINEINFO_INIT;
    JsonLogic_Handle tests = jsonlogic_parse_sized(tests_json, stbuf.st_size, &info);

    JsonLogic_Error error = jsonlogic_get_error(tests);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error_sized(stderr, tests_json, stbuf.st_size, error, info);
        free(tests_json);
        return 1;
    }
    free(tests_json);
    tests_json = NULL;

    int status = 0;

    JsonLogic_Iterator iter = jsonlogic_iter(tests);

    bool newline = true;
    size_t test_count = 0;
    size_t pass_count = 0;
    #define FAIL() \
        if (!newline) { \
            puts(" FAILED"); \
            newline = true; \
        }
    for (;;) {
        JsonLogic_Handle test = jsonlogic_iter_next(&iter);

        if (test == JSONLOGIC_ERROR_STOP_ITERATION) {
            break;
        }

        if (jsonlogic_is_string(test)) {
            if (!newline) {
                puts(" OK");
            }
            size_t size = 0;
            const char16_t *str = jsonlogic_get_string_content(test, &size);
            printf(" - "); jsonlogic_print_utf16(stdout, str, size);
            fflush(stdout);
            newline = false;
        } else {
            ++ test_count;

            JsonLogic_Handle logic    = jsonlogic_get(test, jsonlogic_number_from(0));
            JsonLogic_Handle data     = jsonlogic_get(test, jsonlogic_number_from(1));
            JsonLogic_Handle expected = jsonlogic_get(test, jsonlogic_number_from(2));

            if (jsonlogic_is_error(logic)) {
                FAIL();
                fprintf(stderr, "*** error: in tests JSON: %s\n", jsonlogic_get_error_message(logic));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(data)) {
                FAIL();
                fprintf(stderr, "*** error: in tests JSON: %s\n", jsonlogic_get_error_message(data));
                goto test_cleanup;
            }

            if (jsonlogic_is_error(expected)) {
                FAIL();
                fprintf(stderr, "*** error: in tests JSON: %s\n", jsonlogic_get_error_message(expected));
                goto test_cleanup;
            }

            JsonLogic_Handle actual = jsonlogic_apply(logic, data);

            if (!jsonlogic_deep_strict_equal(expected, actual)) {
                FAIL();
                fprintf(stderr, "*** error: Wrong result\n");
                fprintf(stderr, "    logic: "); jsonlogic_print(stderr, logic);    fputc('\n', stderr);
                fprintf(stderr, "     data: "); jsonlogic_print(stderr, data);     fputc('\n', stderr);
                fprintf(stderr, " expected: "); jsonlogic_print(stderr, expected); fputc('\n', stderr);
                fprintf(stderr, "   actual: "); jsonlogic_print(stderr, actual);   fputc('\n', stderr);
                fputc('\n', stderr);
                goto test_cleanup;
            }

            ++ pass_count;

        test_cleanup:
            jsonlogic_decref(logic);
            jsonlogic_decref(data);
            jsonlogic_decref(expected);
        }

        jsonlogic_decref(test);
    }

    jsonlogic_iter_free(&iter);

    printf("\ntests: %" PRIuPTR ", failed: %" PRIuPTR ", passed: %" PRIuPTR "\n",
        test_count, test_count - pass_count, pass_count);

    jsonlogic_decref(tests);

    return status;
}
