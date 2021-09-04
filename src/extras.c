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
#include <assert.h>
#include <stdarg.h>
#include <float.h>

#if defined(_WIN32) || defined(_WIN64)
    // #include <windows.h>
#else
    #include <sys/time.h>
#endif

#define IS_NUM(CH) ((CH) >= u'0' && (CH) <= u'9')

const char16_t *jsonlogic_parse_uint(const char16_t *str, const char16_t *endptr, uint32_t *value_ptr) {
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

    if (value_ptr) {
        *value_ptr = value;
    }

    return ptr;
}

size_t jsonlogic_scan_uint32(const char16_t **str, const char16_t *endptr, const char16_t *delims, ...) {
    assert(str != NULL && *str != NULL);

    va_list ap;

    va_start(ap, delims);

    size_t count = 0;
    const char16_t *ptr = *str;

    while (ptr < endptr) {
        uint32_t *value_ptr = va_arg(ap, uint32_t*);
        assert(value_ptr != NULL);

        const char16_t *prev = ptr;
        ptr = jsonlogic_parse_uint(ptr, endptr, value_ptr);

        if (prev == ptr) {
            break;
        }

        count ++;

        if (!*delims || *ptr != *delims || ptr == endptr) {
            break;
        }

        ptr ++;
        delims ++;
    }

    va_end(ap);

    *str = ptr;

    return count;
}

#define DOUBLE_ILLEGAL_ARGUMENT ((JsonLogic_Handle){ .intptr = JSONLOGIC_ERROR_ILLEGAL_ARGUMENT }.number)

typedef struct JsonLogic_DateTime {
    uint32_t year  ;
    uint32_t month ;
    uint32_t day   ;
    uint32_t hour  ;
    uint32_t minute;
    uint32_t second;
    uint32_t msec  ;
    int32_t  tzoff ;
} JsonLogic_DateTime;

JsonLogic_Error jsonlogic_parse_date_time_utf16_intern(const char16_t *str, size_t size, JsonLogic_DateTime *date_time_ptr) {
    // Parses rfc3339 date-time strings, but allow more parts to be optional.
    // In particular allow date-only, optional seconds, and optional time-zone offset.
    // In rfc3339 only milliseconds are optional.
    // YYYY-MM-DD['T'hh:mm[:ss[.sss]]['Z'|(+|-)[ZZ:ZZ]]]
    assert(date_time_ptr != NULL);

    uint32_t year   = 0;
    uint32_t month  = 0;
    uint32_t day    = 0;
    uint32_t hour   = 0;
    uint32_t minute = 0;
    uint32_t second = 0;
    uint32_t msec   = 0;
    int32_t  tzoff  = 0;

    const char16_t *ptr = str;
    const char16_t *endptr = str + size;
    size_t count = jsonlogic_scan_uint32(&ptr, endptr, u"--", &year, &month, &day);
    if (count != 3) {
        JSONLOGIC_DEBUG_UTF16("illegal date-time string, parsed %" PRIuPTR " values of date component", str, size, count);
        return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
    }

    if (ptr < endptr) {
        char16_t ch = *ptr;
        if ((ch != 'T' && ch != ' ' && ch != 't') || ptr + 1 == endptr) {
            JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
            return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
        }
        ptr ++;

        count = jsonlogic_scan_uint32(&ptr, endptr, u"::", &hour, &minute, &second);
        if (count != 3) {
            JSONLOGIC_DEBUG_UTF16("illegal date-time string, parsed %" PRIuPTR " values of time component", str, size, count);
            return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
        }

        if (ptr < endptr && *ptr == '.') {
            ptr ++;
            const char16_t *next = jsonlogic_parse_uint(ptr, endptr, &msec);
            if (next - ptr != 3) {
                JSONLOGIC_DEBUG_UTF16("illegal date-time string, parsed %" PRIuPTR " characters of milliseconds", str, size, (size_t)(next - ptr));
                return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
            }
            ptr = next;
        }

        if (ptr < endptr) {
            ch = *ptr;
            if (ch == 'Z' || ch == 'z') {
                ptr ++;
            } else if (ch == '+' || ch == '-') {
                int32_t fact = ch == '-' ? -60 : 60;
                uint32_t tzhour   = 0;
                uint32_t tzminute = 0;

                ptr ++;
                count = jsonlogic_scan_uint32(&ptr, endptr, u":", &tzhour, &tzminute);
                if (count != 2) {
                    JSONLOGIC_DEBUG_UTF16("illegal date-time string, parsed %" PRIuPTR " values of time-zone component", str, size, count);
                    return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
                }

                tzoff = fact * (tzhour * 60 + tzminute);
            }
        }

        if (ptr < endptr) {
            size_t index = (size_t)(ptr - str);
            JSONLOGIC_DEBUG_UTF16("illegal date-time string, unexpected characters at index %" PRIuPTR, str, size, index);
            return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
        }
    }

    if (month == 0 || month > 12 || day == 0 || day > 31 || year < 1900 || hour > 24 || minute > 59 || second > 60 || (month != 12 && day != 31 && second > 59)) {
        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
        return JSONLOGIC_ERROR_ILLEGAL_ARGUMENT;
    }

    date_time_ptr->year   = year;
    date_time_ptr->month  = month;
    date_time_ptr->day    = day;
    date_time_ptr->hour   = hour;
    date_time_ptr->minute = minute;
    date_time_ptr->second = second;
    date_time_ptr->msec   = msec;
    date_time_ptr->tzoff  = tzoff;

    return JSONLOGIC_ERROR_SUCCESS;
}

double jsonlogic_parse_date_time_utf16(const char16_t *str, size_t size) {
    JsonLogic_DateTime date_time;

    JsonLogic_Error error = jsonlogic_parse_date_time_utf16_intern(str, size, &date_time);
    if (error != JSONLOGIC_ERROR_SUCCESS) {
        return (JsonLogic_Handle){ .intptr = error }.number;
    }

    struct tm time_info;
    memset(&time_info, 0, sizeof(time_info));

    time_info.tm_year  = date_time.year - 1900;
    time_info.tm_mon   = date_time.month - 1;
    time_info.tm_mday  = date_time.day;
    time_info.tm_hour  = date_time.hour;
    time_info.tm_min   = date_time.minute;
    time_info.tm_sec   = date_time.second;
    time_info.tm_isdst = -1;

#if defined(_WIN32) || defined(_WIN64)
    time_t time = _mkgmtime(&time_info);
#else
    time_t time = timegm(&time_info);
#endif

    if (time == (time_t)-1) {
        JSONLOGIC_DEBUG_UTF16("%s", str, size, "illegal date-time string");
        return DOUBLE_ILLEGAL_ARGUMENT;
    }

    return (double)(time - date_time.tzoff) * 1000 + (double)date_time.msec;
}

JsonLogic_Handle jsonlogic_format_date_time(double timestamp) {
    if (!isfinite(timestamp)) {
        JSONLOGIC_DEBUG("timestamp is not a finite value: %.*g", DBL_DIG, timestamp);
        return JsonLogic_Error_IllegalArgument;
    }

    // YYYY-MM-DDTHH:mm:ss.sssZ
    char buf[25];
    struct tm time_info;
    time_t tv = (int64_t)timestamp / 1000;
    int msec  = (int64_t)timestamp % 1000;
    if (msec < 0) {
        tv  -= 1;
        msec = 1000 + msec;
    }

#if defined(_WIN32) || defined(_WIN64)
    errno_t errnum = gmtime_s(&time_info, &tv);
    if (errnum != 0) {
        JSONLOGIC_DEBUG("failed to convert timestamp %.*g to date-time string: %s", DBL_DIG, timestamp, strerror(errnum));
        return JsonLogic_Error_IllegalArgument;
    }
#else
    if (gmtime_r(&tv, &time_info) == NULL) {
        JSONLOGIC_DEBUG("failed to convert timestamp %.*g to date-time string: %s", DBL_DIG, timestamp, strerror(errno));
        return JsonLogic_Error_IllegalArgument;
    }
#endif

    int count = snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
        time_info.tm_year + 1900,
        time_info.tm_mon + 1,
        time_info.tm_mday,
        time_info.tm_hour,
        time_info.tm_min,
        time_info.tm_sec,
        msec
    );
    assert(count < sizeof(buf));

    return jsonlogic_string_from_latin1(buf);
}

JsonLogic_Handle jsonlogic_extra_FORMAT_TIME(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    JsonLogic_Handle handle = args[0];
    double timestamp;
    if (JSONLOGIC_IS_NUMBER(handle)) {
        timestamp = handle.number;
    } else if (JSONLOGIC_IS_STRING(handle)) {
        const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
        timestamp = jsonlogic_parse_date_time_utf16(string->str, string->size);
    } else {
        return JsonLogic_Error_IllegalArgument;
    }

    return jsonlogic_format_date_time(timestamp);
}

JsonLogic_Handle jsonlogic_extra_ADD_YEARS(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        return JsonLogic_Error_IllegalArgument;
    }

    double years = jsonlogic_to_double(args[1]);
    if (!isfinite(years)) {
        return JsonLogic_Error_IllegalArgument;
    }

    struct tm time_info;
    uint32_t msec = 0;
    int32_t tzoff = 0;

    memset(&time_info, 0, sizeof(time_info));

    JsonLogic_Handle handle = args[0];
    if (JSONLOGIC_IS_NUMBER(handle)) {
        double timestamp = handle.number;

        time_t tv = (int64_t)timestamp / 1000;
        msec = (int64_t)timestamp % 1000;
        if (msec < 0) {
            tv  -= 1;
            msec = 1000 + msec;
        }

#if defined(_WIN32) || defined(_WIN64)
        errno_t errnum = gmtime_s(&time_info, &tv);
        if (errnum != 0) {
            JSONLOGIC_DEBUG("failed to convert timestamp %.*g to date-time string: %s", DBL_DIG, timestamp, strerror(errnum));
            return JsonLogic_Error_IllegalArgument;
        }
#else
        if (gmtime_r(&tv, &time_info) == NULL) {
            JSONLOGIC_DEBUG("failed to convert timestamp %.*g to date-time string: %s", DBL_DIG, timestamp, strerror(errno));
            return JsonLogic_Error_IllegalArgument;
        }
#endif
    } else if (JSONLOGIC_IS_STRING(handle)) {
        JsonLogic_DateTime date_time;
        const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
        JsonLogic_Error error = jsonlogic_parse_date_time_utf16_intern(string->str, string->size, &date_time);
        if (error != JSONLOGIC_ERROR_SUCCESS) {
            return (JsonLogic_Handle){ .intptr = error };
        }

        time_info.tm_year  = date_time.year - 1900;
        time_info.tm_mon   = date_time.month - 1;
        time_info.tm_mday  = date_time.day;
        time_info.tm_hour  = date_time.hour;
        time_info.tm_min   = date_time.minute;
        time_info.tm_sec   = date_time.second;
        time_info.tm_isdst = -1;
        msec  = date_time.msec;
        tzoff = date_time.tzoff;
    } else {
        return JsonLogic_Error_IllegalArgument;
    }

    time_info.tm_year += (int)years;

#if defined(_WIN32) || defined(_WIN64)
    time_t time = _mkgmtime(&time_info);
#else
    time_t time = timegm(&time_info);
#endif

    if (time == (time_t)-1) {
        JSONLOGIC_DEBUG("illegal date-time: %4d-%2d-%2d %2d:%2d:%2d.%3d tzoff=%d",
            time_info.tm_year + 1900,
            time_info.tm_mon + 1,
            time_info.tm_mday,
            time_info.tm_hour,
            time_info.tm_min,
            time_info.tm_sec,
            msec,
            tzoff
        );
        return JsonLogic_Error_IllegalArgument;
    }

    return jsonlogic_number_from((double)(time - tzoff) * 1000 + (double)msec);
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
                item->items[stack_ptr] = jsonlogic_incref(array->items[index]);
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

double jsonlogic_parse_date_time(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_NUMBER(handle)) {
        return handle.number;
    }

    if (!JSONLOGIC_IS_STRING(handle)) {
        return JsonLogic_Error_IllegalArgument.number;
    }

    const JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

    return jsonlogic_parse_date_time_utf16(string->str, string->size);
}

JsonLogic_Handle jsonlogic_extra_NOW(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    return jsonlogic_number_from(jsonlogic_now());
}

JsonLogic_Handle jsonlogic_extra_PARSE_TIME(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    return jsonlogic_number_from(jsonlogic_parse_date_time(args[0]));
}

JsonLogic_Handle jsonlogic_extra_TIME_SINCE(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_Error_IllegalArgument;
    }

    double tv = jsonlogic_parse_date_time(args[0]);

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

    for (size_t index = 1; index < argc; ++ index) {
        JsonLogic_Handle handle = args[index];

        if (!JSONLOGIC_IS_ARRAY(handle)) {
            return JsonLogic_Error_IllegalArgument;
        }

        const JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
        if (array->size < min_len) {
            min_len = array->size;
        }
    }

    JsonLogic_Array *new_array = jsonlogic_array_with_capacity(min_len);
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
        new_array->items[index] = jsonlogic_array_into_handle(item);
    }

    return jsonlogic_array_into_handle(new_array);
}
