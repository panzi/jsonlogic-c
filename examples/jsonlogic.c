#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char *progname = argc > 0 ? argv[0] : "jsonlogic";
        fprintf(stderr, "usage: %s <logic> <data>\n", progname);
        return 1;
    }
    JsonLogic_LineInfo info  = JSONLOGIC_LINEINFO_INIT;
    JsonLogic_Handle logic = jsonlogic_parse(argv[1], &info);
    JsonLogic_Error  error = jsonlogic_get_error(logic);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_print_parse_error(stderr, argv[1], error, info);
        return 1;
    }

    JsonLogic_Handle data = jsonlogic_parse(argv[2], &info);
    error = jsonlogic_get_error(logic);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        jsonlogic_decref(logic);
        jsonlogic_print_parse_error(stderr, argv[2], error, info);
        return 1;
    }

    JsonLogic_Handle result = jsonlogic_apply(logic, data);

    jsonlogic_decref(logic);
    jsonlogic_decref(data);

    error = jsonlogic_get_error(result);

    if (error != JSONLOGIC_ERROR_SUCCESS) {
        fprintf(stderr, "*** error: %s\n", jsonlogic_get_error_message(error));
        return 1;
    }

    char *utf8 = jsonlogic_stringify_utf8(result);
    jsonlogic_decref(result);

    if (utf8 == NULL) {
        fprintf(stderr,
            "*** error: calling jsonlogic_stringify_utf8(): %s\n",
            strerror(errno));
        return 1;
    } else {
        puts(utf8);
        free(utf8);
    }

    return 0;
}
