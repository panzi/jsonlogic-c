#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int status = 0;
    for (int argind = 1; argind < argc; ++ argind) {
        JsonLogic_Handle value = jsonlogic_parse(argv[argind]);
        JsonLogic_Error error = jsonlogic_get_error(value);
        if (error != JSONLOGIC_ERROR_SUCCESS) {
            status = 1;
            fprintf(stderr, "*** error: parsing JSON: %s: %s\n",
                jsonlogic_get_error_message(error), argv[argind]);
        } else {
            char *utf8 = jsonlogic_stringify_utf8(value);
            if (utf8 == NULL) {
                fprintf(stderr, "*** error: calling jsonlogic_stringify_utf8(): %s\n", argv[argind]);
            } else {
                puts(utf8);
                free(utf8);
            }
        }
        jsonlogic_decref(value);
    }
    return status;
}
