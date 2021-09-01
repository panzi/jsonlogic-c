#include "jsonlogic_intern.h"

#include <errno.h>
#include <inttypes.h>
#include <assert.h>

static int jsonlogic_operation_entry_compare(const void *leftptr, const void *rightptr) {
    const JsonLogic_Operation_Entry *left  = leftptr;
    const JsonLogic_Operation_Entry *right = rightptr;

    return jsonlogic_utf16_compare(left->key, left->key_size, right->key, right->key_size);
}

JsonLogic_Operation jsonlogic_operations_get(const JsonLogic_Operation_Entry *operations, size_t count, const char16_t *key, size_t key_size) {
    if (count == 0) {
        return NULL;
    }

    assert(operations != NULL);

    size_t left  = 0;
    size_t right = count;
    while (left < right) {
        size_t mid = (left + right - 1) / 2;
        const JsonLogic_Operation_Entry *entry = &operations[mid];
        int cmp = jsonlogic_utf16_compare(
            entry->key, entry->key_size,
            key, key_size);
        if (cmp < 0) {
            left  = mid + 1;
        } else if (cmp > 0) {
            right = mid;
        } else {
            return entry->operation;
        }
    }

    return NULL;
}

void jsonlogic_operations_sort(JsonLogic_Operation_Entry *operations, size_t count) {
    qsort(operations, count, sizeof(JsonLogic_Operation_Entry), jsonlogic_operation_entry_compare);
}

JsonLogic_Operation_Entry *jsonlogic_operations_merge(const JsonLogic_Operations ops[], size_t ops_size, size_t *new_size_ptr) {
    if (new_size_ptr == NULL) {
        JSONLOGIC_DEBUG("%s", "illegal argument: new_size_ptr is NULL");
        errno = EINVAL;
        return NULL;
    }

    if (ops_size > 0 && ops == NULL) {
        JSONLOGIC_DEBUG("%s", "illegal argument: ops_size > 0, but ops == NULL");
        errno = EINVAL;
        return NULL;
    }

    size_t new_size = 0;
    for (size_t ops_index = 0; ops_index < ops_size; ++ ops_index) {
        size_t size = ops[ops_index].size;
        if (SIZE_MAX - size < new_size) {
            JSONLOGIC_DEBUG("resulting array would be bigger than SIZE_MAX: %" PRIuPTR " + %" PRIuPTR " > %" PRIuPTR, new_size, size, SIZE_MAX);
            errno = ENOMEM;
            return NULL;
        }
        new_size += size;
    }

    if (SIZE_MAX / sizeof(JsonLogic_Operation_Entry) < new_size) {
        JSONLOGIC_DEBUG("resulting array would need more than SIZE_MAX memory: %" PRIuPTR " * %" PRIuPTR " > %" PRIuPTR, new_size, sizeof(JsonLogic_Operation_Entry), SIZE_MAX);
        errno = ENOMEM;
        return NULL;
    }

    JsonLogic_Operation_Entry *new_entries = malloc(new_size * sizeof(JsonLogic_Operation_Entry));
    if (new_entries == NULL) {
        JSONLOGIC_ERROR_MEMORY();
        errno = ENOMEM;
        return NULL;
    }

    size_t new_index = 0;
    for (size_t ops_index = 0; ops_index < ops_size; ++ ops_index) {
        const JsonLogic_Operations *this_ops = &ops[ops_index];

        for (size_t index = 0; index < this_ops->size; ++ index) {
            const JsonLogic_Operation_Entry *entry = &this_ops->entries[index];

            bool found = false;
            for (size_t next_ops_index = ops_index + 1; next_ops_index < ops_size; ++ next_ops_index) {
                const JsonLogic_Operations *other_ops = &ops[next_ops_index];
                if (jsonlogic_operations_get(other_ops->entries, other_ops->size, entry->key, entry->key_size) != NULL) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                new_entries[new_index ++] = *entry;
            }
        }
    }

    assert(new_index <= new_size);

    if (new_index < new_size) {
        new_size = new_index;
        JsonLogic_Operation_Entry *truncated_entries = realloc(new_entries, sizeof(JsonLogic_Operation_Entry) * new_size);
        if (truncated_entries == NULL) {
            JSONLOGIC_ERROR("%s", "failed to truncate operations array");
        } else {
            new_entries = truncated_entries;
        }
    }

    jsonlogic_operations_sort(new_entries, new_size);

    *new_size_ptr = new_size;

    return new_entries;
}
