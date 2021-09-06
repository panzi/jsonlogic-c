#include "jsonlogic_intern.h"

JsonLogic_Handle certlogic_apply(JsonLogic_Handle logic, JsonLogic_Handle input) {
    return certlogic_apply_custom(logic, input, &CertLogic_Builtins);
}

JsonLogic_Handle certlogic_op_NOT(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_True;
    }
    return certlogic_not(args[0]);
}

JsonLogic_Handle certlogic_op_TO_BOOL(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc == 0) {
        return JsonLogic_False;
    }
    return certlogic_to_boolean(args[0]);
}

#define JSONLOGIC_COMPILE_CERTLOGIC
#define jsonlogic_apply        certlogic_apply
#define jsonlogic_apply_custom certlogic_apply_custom
#define jsonlogic_to_bool      certlogic_to_bool
#define jsonlogic_to_boolean   certlogic_to_boolean
#define jsonlogic_not          certlogic_not
#include "apply.c"
#undef jsonlogic_apply
#undef jsonlogic_apply_custom
#undef jsonlogic_to_bool
#undef jsonlogic_to_boolean
#undef jsonlogic_not
