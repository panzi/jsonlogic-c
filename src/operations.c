#include "jsonlogic_intern.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

void jsonlogic_operations_debug(const JsonLogic_Operations *operations) {
    fprintf(stderr, "operations: used=%" PRIuPTR " capacity=%" PRIuPTR "\n", operations->used, operations->capacity);
    for (size_t index = 0; index < operations->capacity; ++ index) {
        const JsonLogic_Operation_Entry *entry = &operations->entries[index];
        if (entry->key != NULL) {
            fprintf(stderr, "    index=%4" PRIuPTR " hash=0x%016" PRIx64 " key=\"", index, entry->hash);
            jsonlogic_print_utf16(stderr, entry->key, entry->key_size);
            fprintf(stderr, "\"\n");
        }
    }
}

const JsonLogic_Operation *jsonlogic_operations_get_with_hash(const JsonLogic_Operations *operations, uint64_t hash, const char16_t *key, size_t key_size) {
    if (operations->used == 0) {
        return NULL;
    }
    size_t capacity = operations->capacity;
    size_t index = hash % capacity;
    size_t start_index = index;

    do {
        const JsonLogic_Operation_Entry *entry = &operations->entries[index];

        if (entry->key == NULL) {
            break;
        }

        if (jsonlogic_utf16_equals(key, key_size, entry->key, entry->key_size)) {
            return &entry->operation;
        }

        index = (index + 1) % capacity;
    } while (index != start_index);

    return NULL;
}

JsonLogic_Error jsonlogic_operations_set_with_hash(JsonLogic_Operations *operations, uint64_t hash, const char16_t *key, size_t key_size, void *context, JsonLogic_Operation_Funct funct) {
    assert(operations != NULL);
    JsonLogic_Operation_Entry *entries = operations->entries;
    if (entries == NULL) {
        assert(operations->used == 0 && operations->capacity == 0);
        size_t new_capacity = 16;
        JsonLogic_Operation_Entry *new_entries = calloc(new_capacity, sizeof(JsonLogic_Operation_Entry));
        if (new_entries == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }

        size_t index = hash % new_capacity;
        new_entries[index] = (JsonLogic_Operation_Entry) {
            .hash      = hash,
            .key       = key,
            .key_size  = key_size,
            .operation = {
                .context = context,
                .funct   = funct,
            }
        };

        operations->entries  = new_entries;
        operations->capacity = new_capacity;
        operations->used     = 1;
    } else {
        size_t capacity = operations->capacity;
        size_t index = hash % capacity;
        size_t start_index = index;

        do {
            JsonLogic_Operation_Entry *entry = &entries[index];

            if (entry->key == NULL) {
                if (operations->used + 1 > capacity / 2) {
                    // resize and insert instead
                    break;
                }

                *entry = (JsonLogic_Operation_Entry) {
                    .hash      = hash,
                    .key       = key,
                    .key_size  = key_size,
                    .operation = {
                        .context = context,
                        .funct   = funct,
                    }
                };

                ++ operations->used;
                return JSONLOGIC_ERROR_SUCCESS;
            }

            if (jsonlogic_utf16_equals(key, key_size, entry->key, entry->key_size)) {
                *entry = (JsonLogic_Operation_Entry) {
                    .hash      = hash,
                    .key       = key,
                    .key_size  = key_size,
                    .operation = {
                        .context = context,
                        .funct   = funct,
                    }
                };
                return JSONLOGIC_ERROR_SUCCESS;
            }

            index = (index + 1) % capacity;
        } while (index != start_index);

        size_t new_capacity = capacity * 2;
        JsonLogic_Operation_Entry *new_entries = calloc(new_capacity, sizeof(JsonLogic_Operation_Entry));
        if (new_entries == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }

        for (size_t index = 0; index < capacity; ++ index) {
            JsonLogic_Operation_Entry *entry = &entries[index];

            if (entry->key != NULL) {
                size_t new_index = entry->hash % new_capacity;
                JSONLOGIC_DEBUG_CODE(size_t start_index = new_index;)

                for (;;) {
                    JsonLogic_Operation_Entry *new_entry = &new_entries[new_index];
                    if (new_entry->key == NULL) {
                        new_entries[new_index] = *entry;
                        break;
                    }

                    new_index = (new_index + 1) % new_capacity;
                    assert(new_index != start_index);
                }
            }
        }

        index = hash % new_capacity;
        start_index = index;
        for (;;) {
            JsonLogic_Operation_Entry *entry = &new_entries[index];
            if (entry->key == NULL) {
                *entry = (JsonLogic_Operation_Entry) {
                    .hash      = hash,
                    .key       = key,
                    .key_size  = key_size,
                    .operation = {
                        .context = context,
                        .funct   = funct,
                    }
                };
                break;
            }

            if (jsonlogic_utf16_equals(key, key_size, entry->key, entry->key_size)) {
                *entry = (JsonLogic_Operation_Entry) {
                    .hash      = hash,
                    .key       = key,
                    .key_size  = key_size,
                    .operation = {
                        .context = context,
                        .funct   = funct,
                    }
                };
                break;
            }

            index = (index + 1) % new_capacity;
            assert(index != start_index);
        }

        free(entries);
        operations->capacity = new_capacity;
        operations->entries = new_entries;
        ++ operations->used;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

const JsonLogic_Operation *jsonlogic_operations_get_sized(const JsonLogic_Operations *operations, const char16_t *key, size_t key_size) {
    return jsonlogic_operations_get_with_hash(operations, jsonlogic_hash_fnv1a_utf16(key, key_size), key, key_size);
}

JsonLogic_Error jsonlogic_operations_set_sized(JsonLogic_Operations *operations, const char16_t *key, size_t key_size, void *context, JsonLogic_Operation_Funct funct) {
    return jsonlogic_operations_set_with_hash(operations, jsonlogic_hash_fnv1a_utf16(key, key_size), key, key_size, context, funct);
}

JsonLogic_Error jsonlogic_operations_extend(JsonLogic_Operations *operations, const JsonLogic_Operations *more_operations) {
    assert(operations != NULL && more_operations != NULL);

    if (operations->entries == NULL) {
        assert(operations->capacity == 0 && operations->used == 0);

        size_t byte_size = more_operations->capacity * sizeof(JsonLogic_Operation_Entry);
        JsonLogic_Operation_Entry *new_entries = malloc(byte_size);
        if (new_entries == NULL) {
            JSONLOGIC_ERROR_MEMORY();
            return JSONLOGIC_ERROR_OUT_OF_MEMORY;
        }

        memcpy(new_entries, more_operations->entries, byte_size);

        operations->entries  = new_entries;
        operations->capacity = more_operations->capacity;
        operations->used     = more_operations->used;

        return JSONLOGIC_ERROR_SUCCESS;
    }

    for (size_t index = 0; index < more_operations->capacity; ++ index) {
        const JsonLogic_Operation_Entry *entry = &more_operations->entries[index];
        if (entry->key != NULL) {
            JsonLogic_Error error = jsonlogic_operations_set_with_hash(operations, entry->hash, entry->key, entry->key_size, entry->operation.context, entry->operation.funct);
            if (error != JSONLOGIC_ERROR_SUCCESS) {
                return error;
            }
        }
    }

    return JSONLOGIC_ERROR_SUCCESS;
}

JsonLogic_Error jsonlogic_operations_build(JsonLogic_Operations *operations, const JsonLogic_Operations_BuildEntry *build_operations) {
    const JsonLogic_Operations_BuildEntry *entry = build_operations;
    while (entry->key != NULL) {
        size_t key_size = jsonlogic_utf16_len(entry->key);
        uint64_t hash = jsonlogic_hash_fnv1a_utf16(entry->key, key_size);
        JsonLogic_Error error = jsonlogic_operations_set_with_hash(operations, hash, entry->key, key_size, entry->operation.context, entry->operation.funct);
        if (error != JSONLOGIC_ERROR_SUCCESS) {
            return error;
        }
        ++ entry;
    }
    return JSONLOGIC_ERROR_SUCCESS;
}

void jsonlogic_operations_free(JsonLogic_Operations *operations) {
    free(operations->entries);
    operations->capacity = 0;
    operations->used     = 0;
    operations->entries  = NULL;
}
