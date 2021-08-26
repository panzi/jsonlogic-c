#include "jsonlogic_intern.h"

#include <stdlib.h>
#include <stdarg.h>

JsonLogic_Handle jsonlogic_empty_array() {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle));
    if (array == NULL) {
        return JsonLogic_Null;
    }

    array->refcount = 1;
    array->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

JsonLogic_Array *jsonlogic_array_with_capacity(size_t size) {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * size);
    if (array == NULL) {
        return NULL;
    }

    array->refcount = 1;
    array->size     = 0;

    for (size_t index = 0; index < size; ++ index) {
        array->items[index] = JsonLogic_Null;
    }

    return array;
}

JsonLogic_Handle jsonlogic_array_from_vararg(size_t count, ...) {
    JsonLogic_Array *array = malloc(sizeof(JsonLogic_Array) - sizeof(JsonLogic_Handle) + sizeof(JsonLogic_Handle) * count);
    if (array == NULL) {
        return JsonLogic_Null;
    }

    array->refcount = 1;
    array->size     = count;

    va_list ap;
    va_start(ap, count);

    for (size_t index = 0; index < count; ++ index) {
        JsonLogic_Handle item = va_arg(ap, JsonLogic_Handle);
        jsonlogic_incref(item);
        array->items[index] = item;
    }

    va_end(ap);

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)array) | JsonLogic_Type_Array };
}

void jsonlogic_array_free(JsonLogic_Array *array) {
    for (size_t index = 0; index < array->size; ++ index) {
        jsonlogic_decref(array->items[index]);
    }
    free(array);
}
