#include "jsonlogic_intern.h"

uint64_t jsonlogic_hash_fnv1a(const uint8_t *data, size_t size) {
    uint64_t hash = 0xcbf29ce484222325;

    for (size_t index = 0; index < size; ++ index) {
        hash ^= data[index];
        hash *= 0x00000100000001B3;
    }

    return hash;
}

uint64_t jsonlogic_hash_fnv1a_utf16(const char16_t *str, size_t size) {
    uint64_t hash = 0xcbf29ce484222325;

    for (size_t index = 0; index < size; ++ index) {
        char16_t ch = str[index];

        hash ^= (uint8_t)(ch & 0xFF);
        hash *= 0x00000100000001B3;

        hash ^= (uint8_t)(ch >> 8);
        hash *= 0x00000100000001B3;
    }

    return hash;
}
