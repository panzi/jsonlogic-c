#include "jsonlogic_intern.h"

#include <inttypes.h>
#include <string.h>
#include <assert.h>

const JsonLogic_Handle JsonLogic_Error_Success          = JSONLOGIC_ERROR_SUCCESS          ;
const JsonLogic_Handle JsonLogic_Error_OutOfMemory      = JSONLOGIC_ERROR_OUT_OF_MEMORY    ;
const JsonLogic_Handle JsonLogic_Error_IllegalOperation = JSONLOGIC_ERROR_ILLEGAL_OPERATION;
const JsonLogic_Handle JsonLogic_Error_IllegalArgument  = JSONLOGIC_ERROR_ILLEGAL_ARGUMENT ;
const JsonLogic_Handle JsonLogic_Error_InternalError    = JSONLOGIC_ERROR_INTERNAL_ERROR   ;
const JsonLogic_Handle JsonLogic_Error_StopIteration    = JSONLOGIC_ERROR_STOP_ITERATION   ;
const JsonLogic_Handle JsonLogic_Error_IOError          = JSONLOGIC_ERROR_IO_ERROR         ;
const JsonLogic_Handle JsonLogic_Error_SyntaxError      = JSONLOGIC_ERROR_SYNTAX_ERROR     ;
const JsonLogic_Handle JsonLogic_Error_UnicodeError     = JSONLOGIC_ERROR_UNICODE_ERROR    ;

JsonLogic_Error jsonlogic_get_error(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_ERROR(handle)) {
        return (JsonLogic_Error) handle;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

const char *jsonlogic_get_error_message(JsonLogic_Error error) {
    switch (error) {
        case JSONLOGIC_ERROR_SUCCESS:
            return "Success";

        case JSONLOGIC_ERROR_OUT_OF_MEMORY:
            return "Out of Memory";

        case JSONLOGIC_ERROR_ILLEGAL_OPERATION:
            return "Illegal Operation";

        case JSONLOGIC_ERROR_ILLEGAL_ARGUMENT:
            return "Illegal Argument";

        case JSONLOGIC_ERROR_INTERNAL_ERROR:
            return "Internal Error (this is a bug)";

        case JSONLOGIC_ERROR_STOP_ITERATION:
            return "Stop Iteration";

        case JSONLOGIC_ERROR_IO_ERROR:
            return "Input/Output Error";

        case JSONLOGIC_ERROR_SYNTAX_ERROR:
            return "Syntax Error";

        case JSONLOGIC_ERROR_UNICODE_ERROR:
            return "Unicode Error";

        default:
            JSONLOGIC_DEBUG("illegal error code: 0x%" PRIx64, error);
            return "(Illegal Error Code)";
    }
}

JsonLogic_Handle jsonlogic_error_from(JsonLogic_Error error) {
    switch (error) {
        case JSONLOGIC_ERROR_SUCCESS:
        case JSONLOGIC_ERROR_OUT_OF_MEMORY:
        case JSONLOGIC_ERROR_ILLEGAL_OPERATION:
        case JSONLOGIC_ERROR_ILLEGAL_ARGUMENT:
        case JSONLOGIC_ERROR_INTERNAL_ERROR:
        case JSONLOGIC_ERROR_STOP_ITERATION:
        case JSONLOGIC_ERROR_IO_ERROR:
        case JSONLOGIC_ERROR_SYNTAX_ERROR:
        case JSONLOGIC_ERROR_UNICODE_ERROR:
            return error;

        default:
            return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
    }
}

void jsonlogic_print_parse_error(FILE *stream, const char *str, JsonLogic_Error error, JsonLogic_LineInfo info) {
    jsonlogic_print_parse_error_sized(stream, str, strlen(str), error, info);
}

const char *jsonlogic_get_linestart(const char *str, size_t index) {
    const char *linestart = str + index;
    while (linestart > str) {
        const char *prev = linestart - 1;
        if (*prev == '\n')
            break;
        linestart = prev;
    }
    return linestart;
}

void jsonlogic_print_parse_error_sized(FILE *stream, const char *str, size_t size, JsonLogic_Error error, JsonLogic_LineInfo info) {
    fprintf(stderr, "*** error: parsing JSON in line %" PRIuPTR " and column %" PRIuPTR ":\n\n",
        info.lineno, info.column);

    const char *linestart = jsonlogic_get_linestart(str, info.index);
    const char *end = str + size;
    const char *lineend = str + info.index;
    while (lineend < end) {
        if (*lineend == '\n')
            break;
        ++ lineend;
    }

    int padding = snprintf(NULL, 0, "%" PRIuPTR, info.lineno);
    assert(padding > 0);

    if (info.lineno > 1) {
        const char *prevline = jsonlogic_get_linestart(str, linestart - str - 1);
        fprintf(stream, " %*" PRIuPTR " | ", padding, info.lineno - 1);
        fwrite(prevline, 1, linestart - prevline, stream);
    }

    fprintf(stream, " %" PRIuPTR " | ", info.lineno);
    fwrite(linestart, 1, lineend - linestart, stream);
    fputc('\n', stream);

    fprintf(stream, " %*s   ", padding, "");
    for (size_t col = 1; col < info.column; ++ col) {
        fputc('-', stream);
    }
    fprintf(stream, "^\n");
    fprintf(stream, " %*s   %s\n", padding, "", jsonlogic_get_error_message(error));
}
