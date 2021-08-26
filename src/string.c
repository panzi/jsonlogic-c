#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

JsonLogic_Handle jsonlogic_string_from_latin1(const char *str) {
    return jsonlogic_string_from_latin1_sized(str, strlen(str));
}

JsonLogic_Handle jsonlogic_string_from_latin1_sized(const char *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * size);
    if (string == NULL) {
        return JsonLogic_Null;
    }
    string->refcount = 1;
    string->size     = size;

    for (size_t index = 0; index < size; ++ index) {
        string->str[index] = str[index];
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_string_from_utf16(const JsonLogic_Char *str) {
    const JsonLogic_Char *ptr = str;
    while (*ptr) ++ ptr;
    size_t size = ptr - str;
    return jsonlogic_string_from_utf16_sized(str, size);
}

JsonLogic_Handle jsonlogic_string_from_utf16_sized(const JsonLogic_Char *str, size_t size) {
    JsonLogic_String *string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * size);
    if (string == NULL) {
        return JsonLogic_Null;
    }
    string->refcount = 1;
    string->size     = size;

    memcpy(string->str, str, size * sizeof(JsonLogic_Char));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

static JsonLogic_Handle jsonlogic_string_substr(const JsonLogic_String *string, JsonLogic_Handle index, JsonLogic_Handle size) {
    size_t sz_index;
    size_t sz_size;

    double dbl_index = jsonlogic_to_double(index);
    if (isnan(dbl_index)) {
        sz_index = 0;
    } else if (dbl_index < 0) {
        if (-dbl_index >= (double)string->size) {
            sz_index = 0;
        } else {
            sz_index = string->size + (size_t)dbl_index;
        }
    } else {
        if (dbl_index > (double)string->size) {
            sz_index = string->size;
        } else {
            sz_index = (size_t)dbl_index;
        }
    }

    if (JSONLOGIC_IS_NULL(size)) {
        sz_size = sz_index < string->size ? string->size - sz_index : 0;
    } else {
        double dbl_size = jsonlogic_to_double(size);

        if (isnan(dbl_size)) {
            sz_size = 0;
        } else if (dbl_size < 0) {
            if (-dbl_size >= (double)string->size) {
                sz_size = 0;
            } else {
                sz_size = string->size + (size_t)dbl_size;
            }
        } else {
            if (dbl_size > (double)(string->size - sz_index)) {
                sz_size = string->size - sz_index;
            } else {
                sz_size = (size_t)dbl_size;
            }
        }
    }

    JsonLogic_String *new_string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * sz_size);

    if (new_string == NULL) {
        return JsonLogic_Null;
    }

    new_string->refcount = 1;
    new_string->size     = sz_size;

    memcpy(new_string->str, string->str + sz_index, sz_size * sizeof(JsonLogic_Char));

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)new_string) | JsonLogic_Type_String };
}

JsonLogic_Handle jsonlogic_substr(JsonLogic_Handle handle, JsonLogic_Handle index, JsonLogic_Handle size) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        JsonLogic_Handle temp = jsonlogic_to_string(handle);
        if (JSONLOGIC_IS_NULL(temp)) {
            // memory allocation failed
            return JsonLogic_Null;
        }
        JsonLogic_Handle result = jsonlogic_string_substr(JSONLOGIC_CAST_STRING(temp), index, size);
        jsonlogic_decref(temp);
        return result;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);

    return jsonlogic_string_substr(string, index, size);
}

const JsonLogic_Char *jsonlogic_get_utf16(JsonLogic_Handle handle, size_t *sizeptr) {
    if (!JSONLOGIC_IS_STRING(handle)) {
        return NULL;
    }

    JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
    if (sizeptr != NULL) {
        *sizeptr = string->size;
    }

    return string->str;
}

bool jsonlogic_buffer_ensure(JsonLogic_Buffer *buf, size_t want_free_size) {
    size_t used = buf->data == NULL ? 0 : buf->data->size;
    size_t has_free_size = buf->size - used;

    if (has_free_size < want_free_size) {
        size_t add_size = want_free_size - has_free_size;
        size_t new_size = buf->size + (add_size < JSONLOGIC_CHUNK_SIZE ? JSONLOGIC_CHUNK_SIZE : add_size);
        JsonLogic_String *new_string = realloc(buf->data, sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * new_size);
        if (new_string == NULL) {
            return false;
        }
        buf->data = new_string;
        buf->size = new_size;
    }
    return true;
}

bool jsonlogic_buffer_append_latin1(JsonLogic_Buffer *buf, const char *str) {
    size_t size = strlen(str);
    if (!jsonlogic_buffer_ensure(buf, size)) {
        return false;
    }

    size_t outindex = buf->data->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->data->str[outindex ++] = str[index];
    }
    buf->data->size += size;

    return false;
}

bool jsonlogic_buffer_append_utf16(JsonLogic_Buffer *buf, JsonLogic_Char *str, size_t size) {
    if (!jsonlogic_buffer_ensure(buf, size)) {
        return false;
    }

    size_t outindex = buf->data->size;
    for (size_t index = 0; index < size; ++ index) {
        buf->data->str[outindex ++] = str[index];
    }
    buf->data->size += size;

    return true;
}

#define JSON_LOGIC_OBJECT_STRING "[object Object]"
#define JSON_LOGIC_NULL_STRING   "null"
#define JSON_LOGIC_TRUE_STRING   "true"
#define JSON_LOGIC_FALSE_STRING  "false"

bool jsonlogic_buffer_append(JsonLogic_Buffer *buf, JsonLogic_Handle handle) {
    if (handle.intptr < JsonLogic_MaxNumber) {
        char latin1[128];
        int count = snprintf(latin1, sizeof(buf), "%g", handle.number);
        if (count >= sizeof(latin1)) {
            size_t size = (size_t)count + 1;
            char *latin1 = malloc(size);
            if (latin1 == NULL) {
                return false;
            }
            snprintf(latin1, size, "%g", handle.number);
            bool result = jsonlogic_buffer_append_latin1(buf, latin1);
            free(latin1);
            return result;
        }
        return jsonlogic_buffer_append_latin1(buf, latin1);
    }

    switch (handle.intptr & JsonLogic_TypeMask) {
        case JsonLogic_Type_String:
        {
            JsonLogic_String *string = JSONLOGIC_CAST_STRING(handle);
            return jsonlogic_buffer_append_utf16(buf, string->str, string->size);
        }
        case JsonLogic_Type_Boolean:
        {
            if (handle.intptr == JsonLogic_False.intptr) {
                JsonLogic_Char str[sizeof(JSON_LOGIC_FALSE_STRING) - 1];
                for (size_t index = 0; index < sizeof(JSON_LOGIC_FALSE_STRING) - 1; ++ index) {
                    str[index] = JSON_LOGIC_FALSE_STRING[index];
                }
                return jsonlogic_buffer_append_utf16(buf, str, sizeof(JSON_LOGIC_FALSE_STRING) - 1);
            } else {
                JsonLogic_Char str[sizeof(JSON_LOGIC_TRUE_STRING) - 1];
                for (size_t index = 0; index < sizeof(JSON_LOGIC_TRUE_STRING) - 1; ++ index) {
                    str[index] = JSON_LOGIC_TRUE_STRING[index];
                }
                return jsonlogic_buffer_append_utf16(buf, str, sizeof(JSON_LOGIC_TRUE_STRING) - 1);
            }
        }
        case JsonLogic_Type_Null:
        {
            JsonLogic_Char str[sizeof(JSON_LOGIC_NULL_STRING) - 1];
            for (size_t index = 0; index < sizeof(JSON_LOGIC_NULL_STRING) - 1; ++ index) {
                str[index] = JSON_LOGIC_NULL_STRING[index];
            }
            return jsonlogic_buffer_append_utf16(buf, str, sizeof(JSON_LOGIC_NULL_STRING) - 1);
        }
        case JsonLogic_Type_Array:
        {
            JsonLogic_Array *array = JSONLOGIC_CAST_ARRAY(handle);
            if (array->size > 0) {
                if (!jsonlogic_buffer_append(buf, array->items[0])) {
                    return false;
                }
                for (size_t index = 1; index < array->size; ++ index) {
                    if (!jsonlogic_buffer_append_utf16(buf, (JsonLogic_Char[]){','}, 1)) {
                        return false;
                    }
                    if (!jsonlogic_buffer_append(buf, array->items[index])) {
                        return false;
                    }
                }
            }
            return true;
        }
        case JsonLogic_Type_Object:
        {
            JsonLogic_Char str[sizeof(JSON_LOGIC_OBJECT_STRING) - 1];
            for (size_t index = 0; index < sizeof(JSON_LOGIC_OBJECT_STRING) - 1; ++ index) {
                str[index] = JSON_LOGIC_OBJECT_STRING[index];
            }
            return jsonlogic_buffer_append_utf16(buf, str, sizeof(JSON_LOGIC_OBJECT_STRING) - 1);
        }
        default:
            return true;
    }
}

JsonLogic_String *jsonlogic_buffer_take(JsonLogic_Buffer *buf) {
    JsonLogic_String *string = buf->data;
    if (string == NULL) {
        string = malloc(sizeof(JsonLogic_String) - sizeof(JsonLogic_Char));
        if (string != NULL) {
            string->refcount = 1;
            string->size     = 0;
        }
    } else {
        // shrink to fit
        string = realloc(string, sizeof(JsonLogic_String) - sizeof(JsonLogic_Char) + sizeof(JsonLogic_Char) * string->size);
        if (string == NULL) {
            // should not happen
            string = buf->data;
        }
    }
    buf->size = 0;
    buf->data = NULL;
    return string;
}

void jsonlogic_buffer_free(JsonLogic_Buffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
}

JsonLogic_Handle jsonlogic_to_string(JsonLogic_Handle handle) {
    if (JSONLOGIC_IS_STRING(handle)) {
        jsonlogic_incref(handle);
        return handle;
    }

    JsonLogic_Buffer buf = JSONLOGIC_BUFFER_INIT;
    if (!jsonlogic_buffer_append(&buf, handle)) {
        jsonlogic_buffer_free(&buf);
        return JsonLogic_Null;
    }

    JsonLogic_String *string = jsonlogic_buffer_take(&buf);
    jsonlogic_buffer_free(&buf);
    if (string == NULL) {
        return JsonLogic_Null;
    }

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)string) | JsonLogic_Type_String };
}

bool jsonlogic_string_equals(const JsonLogic_String *a, const JsonLogic_String *b) {
    if (a->size != b->size) {
        return false;
    }

    return memcmp(a->str, b->str, a->size * sizeof(JsonLogic_Char)) == 0;
}


bool jsonlogic_utf16_equals(const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize) {
    if (asize != bsize) {
        return false;
    }

    return memcmp(a, b, asize * sizeof(JsonLogic_Char)) == 0;
}

int jsonlogic_utf16_compare(const JsonLogic_Char *a, size_t asize, const JsonLogic_Char *b, size_t bsize) {
    size_t minsize = asize < bsize ? asize : bsize;

    for (size_t index = 0; index < minsize; ++ index) {
        JsonLogic_Char ach = a[index];
        JsonLogic_Char bch = b[index];

        int cmp = ach - bch;
        if (cmp != 0) {
            return cmp;
        }
    }

    return asize > bsize ? 1 : asize < bsize ? -1 : 0;
}

int jsonlogic_string_compare(const JsonLogic_String *a, const JsonLogic_String *b) {
    return jsonlogic_utf16_compare(a->str, a->size, b->str, b->size);
}

void jsonlogic_string_free(JsonLogic_String *string) {
    free(string);
}

size_t jsonlogic_string_to_index(const JsonLogic_String *string) {
    size_t value = 0;

    for (size_t index = 0; index < string->size; ++ index) {
        if (value >= SIZE_MAX / 10) {
            return SIZE_MAX;
        }
        JsonLogic_Char ch = string->str[index];
        if (ch < '0' || ch > '1') {
            return SIZE_MAX;
        }
        int ord = ch - '0';
        value *= 10;
        if (value > SIZE_MAX - ord) {
            return SIZE_MAX;
        }
        value += ord;
    }

    return value;
}
