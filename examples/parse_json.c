#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <string.h>
#include <errno.h>

size_t strlen16(const char16_t* strarg) {
   if (strarg == NULL) {
       return 0;
   }
   const char16_t* str = strarg;
   while (*str) ++ str;
   return str - strarg;
}

int main(int argc, char *argv[]) {
    int status = 0;

    for (int argind = 1; argind < argc; ++ argind) {
        JsonLogic_LineInfo info  = JSONLOGIC_LINEINFO_INIT;
        JsonLogic_Handle   value = jsonlogic_parse(argv[argind], &info);
        JsonLogic_Error    error = jsonlogic_get_error(value);
        if (error != JSONLOGIC_ERROR_SUCCESS) {
            status = 1;
            jsonlogic_print_parse_error(stderr, argv[argind], error, info);
        } else {
            error = jsonlogic_stringify_file(stdout, value);

            if (error == JSONLOGIC_ERROR_IO) {
                status = 1;
                fprintf(stderr,
                    "*** error: calling jsonlogic_stringify_file(): %s\n",
                    strerror(errno));
            } else if (error != JSONLOGIC_ERROR_SUCCESS) {
                status = 1;
                fprintf(stderr,
                    "*** error: calling jsonlogic_stringify_file(): %s\n",
                    jsonlogic_get_error_message(error));
            } else {
                putchar('\n');
            }
        }
        jsonlogic_decref(value);
    }
    return status;
}
