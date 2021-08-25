#include "jsonlogic_intern.h"

#include <stdlib.h>

JsonLogic_Handle jsonlogic_empty_object() {
    JsonLogic_Object *object = malloc(sizeof(JsonLogic_Object) - sizeof(JsonLogic_Object_Entry));
    if (object == NULL) {
        return JsonLogic_Null;
    }

    object->refcount = 1;
    object->size     = 0;

    return (JsonLogic_Handle){ .intptr = ((uintptr_t)object) | JsonLogic_Type_Object };
}

void jsonlogic_free_object(JsonLogic_Object *object) {
    for (size_t index = 0; index < object->size; ++ index) {
        jsonlogic_decref(object->items[index].key);
        jsonlogic_decref(object->items[index].value);
    }
    free(object);
}
