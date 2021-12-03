#include "jsonlogic_intern.h"

#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

typedef struct Op {
    const char16_t *name;
    const char *ident;
} Op;

const Op bultin_names[] = {
    { u"!",            "jsonlogic_op_NOT"          },
    { u"!!",           "jsonlogic_op_TO_BOOL"      },
    { u"!=",           "jsonlogic_op_NE"           },
    { u"!==",          "jsonlogic_op_STRICT_NE"    },
    { u"%",            "jsonlogic_op_MOD"          },
    { u"*",            "jsonlogic_op_MUL"          },
    { u"+",            "jsonlogic_op_ADD"          },
    { u"-",            "jsonlogic_op_SUB"          },
    { u"/",            "jsonlogic_op_DIV"          },
    { u"<",            "jsonlogic_op_LT"           },
    { u"<=",           "jsonlogic_op_LE"           },
    { u"==",           "jsonlogic_op_EQ"           },
    { u"===",          "jsonlogic_op_STRICT_EQ"    },
    { u">",            "jsonlogic_op_GT"           },
    { u">=",           "jsonlogic_op_GE"           },
    { u"cat",          "jsonlogic_op_CAT"          },
    { u"in",           "jsonlogic_op_IN"           },
    { u"log",          "jsonlogic_op_LOG"          },
    { u"max",          "jsonlogic_op_MAX"          },
    { u"merge",        "jsonlogic_op_MERGE"        },
    { u"min",          "jsonlogic_op_MIN"          },
    { u"missing",      "jsonlogic_op_MISSING"      },
    { u"missing_some", "jsonlogic_op_MISSING_SOME" },
    { u"substr",       "jsonlogic_op_SUBSTR"       },
    { u"var",          "jsonlogic_op_VAR"          },
    { NULL,            NULL                        },
};

const Op extra_names[] = {
    { u"add-years",       "jsonlogic_extra_ADD_YEARS"         },
    { u"after",           "jsonlogic_extra_AFTER"             },
    { u"before",          "jsonlogic_extra_BEFORE"            },
    { u"combinations",    "jsonlogic_extra_COMBINATIONS"      },
    { u"days",            "jsonlogic_extra_DAYS"              },
    { u"extractFromUVCI", "jsonlogic_extra_EXTRACT_FROM_UVCI" },
    { u"format-time",     "jsonlogic_extra_FORMAT_TIME"       },
    { u"hours",           "jsonlogic_extra_HOURS"             },
    { u"not-after",       "jsonlogic_extra_NOT_AFTER"         },
    { u"not-before",      "jsonlogic_extra_NOT_BEFORE"        },
    { u"now",             "jsonlogic_extra_NOW"               },
    { u"timestamp",       "jsonlogic_extra_PARSE_TIME"        },
    { u"plusTime",        "jsonlogic_extra_PLUS_TIME"         },
    { u"time-since",      "jsonlogic_extra_TIME_SINCE"        },
    { u"to-array",        "jsonlogic_extra_TO_ARRAY"          },
    { u"zip",             "jsonlogic_extra_ZIP"               },
    { NULL,               NULL                                },
};

const Op certlogic_names[] = {
    { u"!",               "certlogic_op_NOT"                  },
    { u"+",               "jsonlogic_op_ADD"                  },
    { u"<",               "jsonlogic_op_LT"                   },
    { u">",               "jsonlogic_op_GT"                   },
    { u"<=",              "jsonlogic_op_LE"                   },
    { u">=",              "jsonlogic_op_GE"                   },
    { u"===",             "jsonlogic_op_STRICT_EQ"            },
    { u"var",             "jsonlogic_op_VAR"                  },
    { u"in",              "jsonlogic_op_IN"                   },
    { u"after",           "jsonlogic_extra_AFTER"             },
    { u"before",          "jsonlogic_extra_BEFORE"            },
    { u"extractFromUVCI", "jsonlogic_extra_EXTRACT_FROM_UVCI" },
    { u"not-after",       "jsonlogic_extra_NOT_AFTER"         },
    { u"not-before",      "jsonlogic_extra_NOT_BEFORE"        },
    { u"plusTime",        "jsonlogic_extra_PLUS_TIME"         },
    { NULL,               NULL                                },
};

const Op certlogic_extra_names[] = {
    { u"!",               "certlogic_op_NOT"                  },
    { u"!!",              "certlogic_op_TO_BOOL"              },
    { u"!=",              "jsonlogic_op_NE"                   },
    { u"!==",             "jsonlogic_op_STRICT_NE"            },
    { u"%",               "jsonlogic_op_MOD"                  },
    { u"*",               "jsonlogic_op_MUL"                  },
    { u"+",               "jsonlogic_op_ADD"                  },
    { u"-",               "jsonlogic_op_SUB"                  },
    { u"/",               "jsonlogic_op_DIV"                  },
    { u"<",               "jsonlogic_op_LT"                   },
    { u"<=",              "jsonlogic_op_LE"                   },
    { u"==",              "jsonlogic_op_EQ"                   },
    { u"===",             "jsonlogic_op_STRICT_EQ"            },
    { u">",               "jsonlogic_op_GT"                   },
    { u">=",              "jsonlogic_op_GE"                   },
    { u"cat",             "jsonlogic_op_CAT"                  },
    { u"in",              "jsonlogic_op_IN"                   },
    { u"log",             "jsonlogic_op_LOG"                  },
    { u"max",             "jsonlogic_op_MAX"                  },
    { u"merge",           "jsonlogic_op_MERGE"                },
    { u"min",             "jsonlogic_op_MIN"                  },
    { u"missing",         "jsonlogic_op_MISSING"              },
    { u"missing_some",    "jsonlogic_op_MISSING_SOME"         },
    { u"substr",          "jsonlogic_op_SUBSTR"               },
    { u"var",             "jsonlogic_op_VAR"                  },
    { u"add-years",       "jsonlogic_extra_ADD_YEARS"         },
    { u"after",           "jsonlogic_extra_AFTER"             },
    { u"before",          "jsonlogic_extra_BEFORE"            },
    { u"combinations",    "jsonlogic_extra_COMBINATIONS"      },
    { u"days",            "jsonlogic_extra_DAYS"              },
    { u"extractFromUVCI", "jsonlogic_extra_EXTRACT_FROM_UVCI" },
    { u"format-time",     "jsonlogic_extra_FORMAT_TIME"       },
    { u"hours",           "jsonlogic_extra_HOURS"             },
    { u"not-after",       "jsonlogic_extra_NOT_AFTER"         },
    { u"not-before",      "jsonlogic_extra_NOT_BEFORE"        },
    { u"now",             "jsonlogic_extra_NOW"               },
    { u"timestamp",       "jsonlogic_extra_PARSE_TIME"        },
    { u"plusTime",        "jsonlogic_extra_PLUS_TIME"         },
    { u"time-since",      "jsonlogic_extra_TIME_SINCE"        },
    { u"to-array",        "jsonlogic_extra_TO_ARRAY"          },
    { u"zip",             "jsonlogic_extra_ZIP"               },
    { NULL,               NULL                                },
};

typedef struct Entry {
    uint64_t hash;
    const char16_t *key;
    size_t key_size;
    const char *ident;
} Entry;

typedef struct Table {
    size_t init_capacity;
    size_t capacity;
    size_t used;
    Entry *entries;
} Table;

#define TABLE_INIT { .init_capacity = 256, .capacity = 0, .used = 0, .entries = NULL }

size_t utf16_len(const char16_t *key) {
    if (key == NULL) {
        return 0;
    }
    const char16_t *ptr = key;
    while (*ptr) ++ ptr;
    return ptr - key;
}

bool table_set(Table *tbl, const char16_t *key, const char *ident) {
    assert(tbl != NULL);

    size_t key_size = utf16_len(key);
    uint64_t hash = jsonlogic_hash_fnv1a_utf16(key, key_size);

    if (tbl->entries == NULL) {
        assert(tbl->used == 0);
        assert(tbl->capacity == 0);

        size_t new_capacity = tbl->init_capacity;
        Entry *new_entries = calloc(new_capacity, sizeof(Entry));
        if (new_entries == NULL) {
            perror("allocating entries");
            return false;
        }
        tbl->capacity = new_capacity;
        tbl->used = 1;

        size_t index = hash % new_capacity;
        new_entries[index] = (Entry) {
            .hash     = hash,
            .key      = key,
            .key_size = key_size,
            .ident    = ident,
        };

        tbl->entries = new_entries;
    } else {
        size_t index = hash % tbl->capacity;
        size_t start_index = index;

        do {
            Entry *entry = &tbl->entries[index];

            if (entry->key == NULL) {
                if (tbl->used + 1 > tbl->capacity / 2) {
                    // resize and insert instead
                    break;
                }
                tbl->entries[index] = (Entry) {
                    .hash     = hash,
                    .key      = key,
                    .key_size = key_size,
                    .ident    = ident,
                };
                ++ tbl->used;
                return true;
            }

            if (hash == entry->hash && key_size == entry->key_size && memcmp(key, entry->key, key_size * sizeof(char16_t)) == 0) {
                tbl->entries[index] = (Entry) {
                    .hash     = hash,
                    .key      = key,
                    .key_size = key_size,
                    .ident    = ident,
                };
                return true;
            }

            index = (index + 1) % tbl->capacity;
        } while (index != start_index);

        size_t new_capacity = tbl->capacity * 2;
        Entry *new_entries = calloc(new_capacity, sizeof(Entry));
        if (new_entries == NULL) {
            perror("allocating entries");
            return false;
        }

        for (size_t index = 0; index < tbl->capacity; ++ index) {
            Entry *entry = &tbl->entries[index];

            if (entry->key != NULL) {
                size_t new_index = entry->hash % new_capacity;
                JSONLOGIC_DEBUG_CODE(const size_t dbg_start_index = new_index;)

                for (;;) {
                    Entry *new_entry = &new_entries[new_index];
                    if (new_entry->key == NULL) {
                        new_entries[new_index] = *entry;
                        break;
                    }

                    new_index = (new_index + 1) % new_capacity;
                    assert(new_index != dbg_start_index);
                }
            }
        }

        index = hash % new_capacity;
        start_index = index;
        for (;;) {
            Entry *entry = &new_entries[index];
            if (entry->key == NULL) {
                new_entries[index] = (Entry) {
                    .hash     = hash,
                    .key      = key,
                    .key_size = key_size,
                    .ident    = ident,
                };
                break;
            }

            if (key_size == entry->key_size && memcmp(key, entry->key, key_size * sizeof(char16_t)) == 0) {
                new_entries[index] = (Entry) {
                    .hash     = hash,
                    .key      = key,
                    .key_size = key_size,
                    .ident    = ident,
                };
                break;
            }

            index = (index + 1) % new_capacity;
            assert(index != start_index);
        }

        free(tbl->entries);
        tbl->entries  = new_entries;
        tbl->capacity = new_capacity;
        ++ tbl->used;
    }

    return true;
}

void table_free(Table *tbl) {
    free(tbl->entries);
    *tbl = (Table)TABLE_INIT;
}

char *concat(const char *str1, const char *str2) {
    size_t size1 = strlen(str1);
    size_t size2 = strlen(str2);
    size_t new_size = size1 + size2 + 1;
    char *new_str = malloc(new_size);
    if (new_str == NULL) {
        return NULL;
    }

    memcpy(new_str, str1, size1);
    memcpy(new_str + size1, str2, size2);
    new_str[new_size - 1] = 0;

    return new_str;
}

void print_ascii_utf16(FILE *fp, const char16_t *str) {
    while (*str) {
        char16_t ch16 = *str;
        assert(ch16 <= 127);
        fputc(ch16, fp);
        ++ str;
    }
}

void generate_source(FILE *fp, const char *symname, const Table *tbl) {
    fprintf(fp, "#include \"jsonlogic_intern.h\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "const JsonLogic_Operations %s = {\n", symname);
    fprintf(fp, "    .capacity = %" PRIuPTR ",\n", tbl->capacity);
    fprintf(fp, "    .used     = %" PRIuPTR ",\n", tbl->used);
    fprintf(fp, "    .entries  = (JsonLogic_Operation_Entry[]) {\n");

    for (size_t index = 0; index < tbl->capacity; ++ index) {
        const Entry *entry = &tbl->entries[index];
        if (entry->key) {
            fprintf(fp, "        { .hash = 0x%" PRIx64 ", .key = u\"", entry->hash);
            print_ascii_utf16(fp, entry->key);
            fprintf(fp,
                "\", .key_size = %" PRIuPTR ", .operation = { .context = NULL, .funct = %s } },\n",
                utf16_len(entry->key), entry->ident);
        } else {
            fprintf(fp, "        { .hash = 0x0, .key = NULL, .key_size = 0, .operation = { .context = NULL, .funct = NULL } },\n");
        }
    }

    fprintf(fp, "    }\n");
    fprintf(fp, "};\n");
}

int main(int argc, char *argv[]) {
    Table tbl = TABLE_INIT;
    int status = 0;
    char *path = NULL;
    FILE *fp = NULL;

    if (argc != 2) {
        fprintf(stderr, "*** error: illegal number of arguments\n");
        goto error;
    }

    const char *outdir = argv[1];

    tbl.init_capacity = 512;
    for (const Op *ptr = bultin_names; ptr->name; ++ ptr) {
        if (!table_set(&tbl, ptr->name, ptr->ident)) {
            fprintf(stderr, "*** error: creating hash table\n");
            goto error;
        }
    }

    path = concat(outdir, "/builtins_tbl.c");
    if (path == NULL) {
        fprintf(stderr, "*** error: allocating memory for path\n");
        goto error;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        goto error;
    }

    generate_source(fp, "JsonLogic_Builtins", &tbl);

    fclose(fp);
    fp = NULL;

    printf("written: %s\n", path);

    free(path);
    path = NULL;

    for (const Op *ptr = extra_names; ptr->name; ++ ptr) {
        if (!table_set(&tbl, ptr->name, ptr->ident)) {
            fprintf(stderr, "*** error: creating hash table\n");
            goto error;
        }
    }

    path = concat(outdir, "/extras_tbl.c");
    if (path == NULL) {
        fprintf(stderr, "*** error: allocating memory for path\n");
        goto error;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        goto error;
    }

    generate_source(fp, "JsonLogic_Extras", &tbl);

    fclose(fp);
    fp = NULL;

    printf("written: %s\n", path);

    free(path);
    path = NULL;

    table_free(&tbl);
    for (const Op *ptr = certlogic_names; ptr->name; ++ ptr) {
        if (!table_set(&tbl, ptr->name, ptr->ident)) {
            fprintf(stderr, "*** error: creating hash table\n");
            goto error;
        }
    }

    path = concat(outdir, "/certlogic_tbl.c");
    if (path == NULL) {
        fprintf(stderr, "*** error: allocating memory for path\n");
        goto error;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        goto error;
    }

    generate_source(fp, "CertLogic_Builtins", &tbl);

    fclose(fp);
    fp = NULL;

    printf("written: %s\n", path);

    free(path);
    path = NULL;

    table_free(&tbl);
    for (const Op *ptr = certlogic_extra_names; ptr->name; ++ ptr) {
        if (!table_set(&tbl, ptr->name, ptr->ident)) {
            fprintf(stderr, "*** error: creating hash table\n");
            goto error;
        }
    }

    path = concat(outdir, "/certlogic_extras_tbl.c");
    if (path == NULL) {
        fprintf(stderr, "*** error: allocating memory for path\n");
        goto error;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror(path);
        goto error;
    }

    generate_source(fp, "CertLogic_Extras", &tbl);

    fclose(fp);
    fp = NULL;

    printf("written: %s\n", path);

    printf("hash of u\"accumulator\": 0x%" PRIx64 "\n", jsonlogic_hash_fnv1a_utf16(u"accumulator", utf16_len(u"accumulator")));
    printf("hash of u\"current\":     0x%" PRIx64 "\n", jsonlogic_hash_fnv1a_utf16(u"current", utf16_len(u"current")));
    printf("hash of u\"data\":        0x%" PRIx64 "\n", jsonlogic_hash_fnv1a_utf16(u"data", utf16_len(u"data")));

    goto cleanup;

error:
    status = 1;

cleanup:
    table_free(&tbl);
    free(path);
    if (fp != NULL) {
        fclose(fp);
    }

    return status;
}
