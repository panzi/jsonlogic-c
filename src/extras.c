// to get timegm()
#define _DEFAULT_SOURCE 1

#include "jsonlogic_intern.h"
#include "jsonlogic_extras.h"

#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
    // #include <windows.h>
#else
    #include <sys/time.h>
#endif

#define JSONLOGIC_EXTRA(NAME) \
    { .key = (JSONLOGIC_##NAME), .key_size = (JSONLOGIC_##NAME##_SIZE), .operation = (jsonlogic_extra_##NAME) }

JSONLOGIC_DEF_UTF16(JSONLOGIC_COMBINATIONS, u"combinations")
JSONLOGIC_DEF_UTF16(JSONLOGIC_DAYS,         u"days")
JSONLOGIC_DEF_UTF16(JSONLOGIC_HOURS,        u"hours")
JSONLOGIC_DEF_UTF16(JSONLOGIC_NOW,          u"now")
JSONLOGIC_DEF_UTF16(JSONLOGIC_PARSE_TIME,   u"parseTime")
JSONLOGIC_DEF_UTF16(JSONLOGIC_TIME_SINCE,   u"timeSince")
JSONLOGIC_DEF_UTF16(JSONLOGIC_ZIP,          u"zip")

static JsonLogic_Handle jsonlogic_extra_COMBINATIONS(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_DAYS        (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_HOURS       (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_NOW         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_PARSE_TIME  (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_TIME_SINCE  (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);
static JsonLogic_Handle jsonlogic_extra_ZIP         (void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc);

#define JSONLOGIC_EXTRAS_COUNT 7
const JsonLogic_Operation_Entry JsonLogic_Extras_Entries[JSONLOGIC_EXTRAS_COUNT] = {
    JSONLOGIC_EXTRA(COMBINATIONS),
    JSONLOGIC_EXTRA(DAYS),
    JSONLOGIC_EXTRA(HOURS),
    JSONLOGIC_EXTRA(NOW),
    JSONLOGIC_EXTRA(PARSE_TIME),
    JSONLOGIC_EXTRA(TIME_SINCE),
    JSONLOGIC_EXTRA(ZIP),
};

const JsonLogic_Operations JsonLogic_Extras = {
    .size = JSONLOGIC_EXTRAS_COUNT,
    .entries = JsonLogic_Extras_Entries,
};

#define IS_NUM(CH) ((CH) >= u'0' && (CH) <= u'9')

const char16_t *jsonlogic_parse_uint(const char16_t *str, const char16_t *endptr, uint32_t *valueptr) {
    uint32_t value = 0;
    const char16_t *ptr = str;

    while (ptr < endptr) {
        char16_t ch = *ptr;
        if (!IS_NUM(ch)) {
            break;
        }

        if (UINT32_MAX / 10 < value) {
            value = UINT32_MAX;
            break;
        }

        value *= 10;
        uint32_t num = ch - u'0';

        if (UINT16_MAX - num < value) {
            value = UINT32_MAX;
            break;
        }

        value += num;

        ++ ptr;
    }

    if (valueptr) {
        *valueptr = value;
    }

    return ptr;
}

#define DBOULE_ILLEGAL_ARGUMENT ((JsonLogic_Handle){ .intptr = JSONLOGIC_ERROR_ILLEGAL_ARGUMENT }.number)

double jsonlogic_parse_date_time(const char16_t *str, size_t size) {
    // Only a subset of ISO 8601 date-time strings are supported:
    // YYYY-MM-DD['T'hh:mm[:ss[.sss]]['Z'|(+|-)[ZZ:ZZ]]]
    // YYYYMMDD['T'hhmm[ss[.sss]]['Z'|(+|-)[ZZZZ]]]

    if (size < 8) {
        JSONLOGIC_DEBUG_UTF16("string is too small for a date-time string (%" PRIuPTR " UTF-16 code units)", str, size, size);
        return DBOULE_ILLEGAL_ARGUMENT;
    }

    struct tm time_info;
    memset(&time_info, 0, sizeof(time_info));

    uint32_t year   = 0;
    uint32_t month  = 0;
    uint32_t day    = 0;
    uint32_t hour   = 0;
    uint32_t minute = 0;
    uint32_t second = 0;
    uint32_t msec   = 0;
    int32_t  tzoff  = 0;

    const char16_t *end  = str + size;
    const char16_t *pos  = str;
    const char16_t *next = pos;

    if (str[4] == u'-') {
        next = jsonlogic_parse_uint(pos, end, &year);
        if (next == pos || next >= end || *next != u'-') {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return DBOULE_ILLEGAL_ARGUMENT;
        }
        pos = next + 1;

        next = jsonlogic_parse_uint(pos, end, &month);
        if (next == pos || next >= end || *next != u'-') {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return DBOULE_ILLEGAL_ARGUMENT;
        }
        pos = next + 1;

        next = jsonlogic_parse_uint(pos, end, &day);
        if (next < end) {
            if (next == pos || *next != u'T') {
                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                return DBOULE_ILLEGAL_ARGUMENT;
            }
            pos = next + 1;

            next = jsonlogic_parse_uint(pos, end, &hour);
            if (next == pos || next >= end || *next != u':') {
                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                return DBOULE_ILLEGAL_ARGUMENT;
            }
            pos = next + 1;

            next = jsonlogic_parse_uint(pos, end, &minute);
            if (next < end) {
                if (next == pos) {
                    JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                    return DBOULE_ILLEGAL_ARGUMENT;
                }

                if (next < end && *next == u':') {
                    pos = next + 1;

                    next = jsonlogic_parse_uint(pos, end, &second);
                    if (next == pos) {
                        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                        return DBOULE_ILLEGAL_ARGUMENT;
                    }

                    if (next < end && *next == u'.') {
                        pos = next + 1;

                        next = jsonlogic_parse_uint(pos, end, &second);
                        if (next != pos + 3) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                    }
                }
                pos = next;

                if (pos < end) {
                    if (*pos == u'Z') {
                        pos ++;
                    } else {
                        int32_t sign = 1;
                        switch (*pos) {
                            case u'+': sign =  1; break;
                            case u'-': sign = -1; break;

                            default:
                                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                                return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos ++;

                        uint32_t tzhour   = 0;
                        uint32_t tzminute = 0;

                        next = jsonlogic_parse_uint(pos, end, &tzhour);
                        if (next != pos + 2 || next >= end || *next != u':') {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos = next + 1;

                        next = jsonlogic_parse_uint(pos, end, &tzminute);
                        if (next != pos + 2) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos = next;

                        tzoff = sign * (int32_t)((tzhour * 60 + tzminute) * 60);
                    }
                }
            }
        }
    } else {
        if (pos + 8 > end) {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return DBOULE_ILLEGAL_ARGUMENT;
        }
        next = jsonlogic_parse_uint(pos, pos + 4, &year);
        if (next != pos + 4 || next >= end) {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return DBOULE_ILLEGAL_ARGUMENT;
        }
        pos = next;

        next = jsonlogic_parse_uint(pos, pos + 2, &month);
        if (next != pos + 2 || next >= end) {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return DBOULE_ILLEGAL_ARGUMENT;
        }
        pos = next;

        next = jsonlogic_parse_uint(pos, pos + 2, &day);
        if (next < end) {
            if (pos + 5 > end) {
                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                return DBOULE_ILLEGAL_ARGUMENT;
            }

            if (next == pos || *next != u'T') {
                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                return DBOULE_ILLEGAL_ARGUMENT;
            }
            pos = next + 1;

            next = jsonlogic_parse_uint(pos, pos + 2, &hour);
            if (next != pos + 2 || next >= end) {
                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                return DBOULE_ILLEGAL_ARGUMENT;
            }
            pos = next;

            next = jsonlogic_parse_uint(pos, pos + 2, &minute);
            if (next < end) {
                if (next == pos) {
                    JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                    return DBOULE_ILLEGAL_ARGUMENT;
                }

                if (IS_NUM(*next)) {
                    pos = next;

                    if (pos + 2 > end) {
                        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                        return DBOULE_ILLEGAL_ARGUMENT;
                    }
                    next = jsonlogic_parse_uint(pos, pos + 2, &second);
                    if (next != pos + 2) {
                        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                        return DBOULE_ILLEGAL_ARGUMENT;
                    }

                    if (next < end && *next == u'.') {
                        pos = next + 1;

                        if (pos + 3 > end) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        next = jsonlogic_parse_uint(pos, pos + 3, &second);
                        if (next != pos + 3) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                    }
                }
                pos = next;

                if (pos < end) {
                    if (*next == u'Z') {
                        pos ++;
                    } else {
                        int32_t sign = 1;
                        switch (*pos) {
                            case u'+': sign =  1; break;
                            case u'-': sign = -1; break;

                            default:
                                JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                                return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos ++;

                        uint32_t tzhour   = 0;
                        uint32_t tzminute = 0;

                        if (pos + 4 > end) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        next = jsonlogic_parse_uint(pos, pos + 2, &tzhour);
                        if (next != pos + 2 || next >= end) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos = next;

                        next = jsonlogic_parse_uint(pos, pos + 2, &tzminute);
                        if (next != pos + 2) {
                            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
                            return DBOULE_ILLEGAL_ARGUMENT;
                        }
                        pos = next;

                        tzoff = sign * (int32_t)((tzhour * 60 + tzminute) * 60);
                    }
                }
            }
        }
    }

    if (pos != end) {
        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
        return DBOULE_ILLEGAL_ARGUMENT;
    }

    if (month == 0 || month > 12 || day == 0 || day > 31 || year < 1900 || hour > 24 || minute > 59 || second > 60 || (month != 12 && day != 31 && second > 59)) {
        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
        return DBOULE_ILLEGAL_ARGUMENT;
    }

    time_info.tm_year  = year - 1900;
    time_info.tm_mon   = month - 1;
    time_info.tm_mday  = day;
    time_info.tm_hour  = hour;
    time_info.tm_min   = minute;
    time_info.tm_sec   = second;
    time_info.tm_isdst = -1;

#if defined(_WIN32) || defined(_WIN64)
    time_t time = _mkgmtime(&time_info);
#else
    time_t time = timegm(&time_info);
#endif

    if (time == (time_t)-1) {
        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
        return DBOULE_ILLEGAL_ARGUMENT;
    }

    return (double)(time - tzoff) * 1000 + msec;
}

JsonLogic_Handle jsonlogic_extra_COMBINATIONS(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return jsonlogic_empty_array();
    }

    JsonLogic_Handle handle = args[0];
    if (!JSONLOGIC_IS_ARRAY(handle)) {
        return jsonlogic_empty_array();
    }
    const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
    size_t prod_len = array->size;

    for (size_t index = 1; index < argc; ++ index) {
        JsonLogic_Handle handle = args[index];
        if (!JSONLOGIC_IS_ARRAY(handle)) {
            return jsonlogic_empty_array();
        }
        prod_len *= JSONLOGIC_CAST_ARRAY(handle)->size;
    }

    if (prod_len == 0) {
        return jsonlogic_empty_array();
    }

    JsonLogic_Array *combinations = jsonlogic_array_with_capacity(prod_len);
    if (combinations == NULL) {
        return JsonLogic_Error_OutOfMemory;
    }

    size_t *stack = calloc(argc + 1, sizeof(size_t));
    if (stack == NULL) {
        jsonlogic_array_free(combinations);
        return JsonLogic_Error_OutOfMemory;
    }

    JsonLogic_Array *item = jsonlogic_array_with_capacity(argc);
    if (item == NULL) {
        jsonlogic_array_free(combinations);
        free(stack);
        return JsonLogic_Error_OutOfMemory;
    }

    size_t combinations_index = 0;
    size_t stack_ptr = 0;
    stack[0] = 0;

    for (;;) {
        if (stack_ptr == argc) {
            combinations->items[combinations_index ++] = jsonlogic_array_into_handle(item);
            assert(stack_ptr > 0);
            -- stack_ptr;

            item = jsonlogic_array_with_capacity(argc);
            if (item == NULL) {
                jsonlogic_array_free(combinations);
                free(stack);
                jsonlogic_array_free(item);
                return JsonLogic_Error_OutOfMemory;
            }
        } else {
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(args[stack_ptr]);
            const size_t index = stack[stack_ptr];

            if (index == array->size) {
                if (stack_ptr == 0) {
                    break;
                }
                -- stack_ptr;
            } else {
                item->items[stack_ptr] = jsonlogic_incref(args[index]);
                stack[stack_ptr] = index + 1;
                ++ stack_ptr;
                stack[stack_ptr] = 0;
            }
        }
    }

    assert(combinations_index == combinations->size);

    jsonlogic_array_free(item);
    free(stack);

    return jsonlogic_array_into_handle(combinations);
}

JsonLogic_Handle jsonlogic_extra_DAYS(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }
    return jsonlogic_number_from(jsonlogic_to_double(args[0]) * 24 * 60 * 60 * 1000);
}

JsonLogic_Handle jsonlogic_extra_HOURS(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }
    return jsonlogic_number_from(jsonlogic_to_double(args[0]) * 60 * 60 * 1000);
}

double jsonlogic_now() {
#if defined(_WIN32) || defined(_WIN64)
    // TODO: find out how to get milliseconds precision time on Windows
    time_t tv = time(NULL);

    if (tv == (time_t)-1) {
        return JsonLogic_Error_InternalError.number;
    }

    return (double)tv * 1000;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return JsonLogic_Error_InternalError.number;
    }

    return (double)tv.tv_sec * 1000 + (double)(tv.tv_usec / 1000);
#endif
}

double jsonlogic_parse_time(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return handle.number;
    }

    if (!JSONLOGIC_IS_STRING(handle)) {
        return JsonLogic_Error_IllegalArgument.number;
    }

    const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

    return jsonlogic_parse_date_time(string->str, string->size);
}

JsonLogic_Handle jsonlogic_extra_NOW(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    return jsonlogic_number_from(jsonlogic_now());
}

JsonLogic_Handle jsonlogic_extra_PARSE_TIME(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    return jsonlogic_number_from(jsonlogic_parse_time(args[0]));
}

JsonLogic_Handle jsonlogic_extra_TIME_SINCE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    double tv = jsonlogic_parse_time(args[0]);

    return jsonlogic_number_from(jsonlogic_now() - tv);
}

JsonLogic_Handle jsonlogic_extra_ZIP(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return jsonlogic_empty_array();
    }

    if (!JSONLOGIC_IS_ARRAY(args[0])) {
        return JsonLogic_Error_IllegalArgument;
    }

    const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(args[0]);
    size_t min_len = array->size;

    for (size_t index = 1; index < array->size; ++ index) {
        JsonLogic_Handle handle = args[index];

        if (!JSONLOGIC_IS_ARRAY(handle)) {
            return JsonLogic_Error_IllegalArgument;
        }

        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
        if (array->size < min_len) {
            min_len = array->size;
        }
    }

    JsonLogic_Array *new_array = jsonlogic_array_with_capacity(argc);
    if (new_array == NULL) {
        return JsonLogic_Error_OutOfMemory;
    }

    for (size_t index = 0; index < min_len; ++ index) {
        JsonLogic_Array *item = jsonlogic_array_with_capacity(argc);
        if (item == NULL) {
            jsonlogic_array_free(new_array);
            return JsonLogic_Error_OutOfMemory;
        }
        for (size_t argind = 0; argind < argc; ++ argind) {
            const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(args[argind]);
            item->items[argind] = jsonlogic_incref(array->items[index]);
        }
    }

    return jsonlogic_array_into_handle(new_array);
}
