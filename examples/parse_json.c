#include "jsonlogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>

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
