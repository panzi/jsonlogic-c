JsonLogic C Implentation
========================

Implementation of [JsonLogic](https://jsonlogic.com/) and
[CertLogic](https://github.com/ehn-dcc-development/dgc-business-rules/tree/main/certlogic/specification)
just for fun. No claims for correctness or speed are made.

It includes a hand crafted JSON parser that is probably also quite slow compared
to other JSON parsers.

This library uses tagged pointers and ref counted garbage collecting, so if
you have any ref-loops you need to break them manually. Though the public API
only allows immutable use of JSON data and as such you can't craft ref-loops
with that anyway.

I've also written a JsonLogic+CertLogic implementation in Python:
[panzi-json-logic](https://github.com/panzi/panzi-json-logic)

CertLogic and Extras
--------------------

This library also implements CertLogic, which is a JsonLogic dialect. It mainly
removes a lot of operations, adds others, and makes empty objects falsy (which
are truthy under JsonLogic).

In addition to that this library provides some optional extra operations in
`jsonlogic_extras.h`. **TODO:** Document those operations.

Usage Example
-------------

```C
#include <jsonlogic.h>

// ...

const char *logic_str = "{\"<\": [{\"var\": \"a\"}, 2]}";
const char *data_str  = "{\"a\": 1}";

// Parse JSON logic and data, unless you build them up in memory by hand
// (e.g. because you use a different, better, JSON parser):
JsonLogic_LineInfo info = JSONLOGIC_LINEINFO_INIT;
JsonLogic_Handle logic = jsonlogic_parse(logic_str, &info);
JsonLogic_Error  error = jsonlogic_get_error(logic);

if (error != JSONLOGIC_ERROR_SUCCESS) {
    jsonlogic_print_parse_error(stderr, logic_str, error, info);
    exit(1);
}

JsonLogic_Handle data = jsonlogic_parse(data_str, &info);
error = jsonlogic_get_error(logic);

if (error != JSONLOGIC_ERROR_SUCCESS) {
    jsonlogic_decref(logic);
    jsonlogic_print_parse_error(stderr, data_str, error, info);
    exit(1);
}

JsonLogic_Handle result = jsonlogic_apply(logic, data);

jsonlogic_println(stdout, result);
// prints: "true\n"

jsonlogic_decref(logic);
jsonlogic_decref(data);
jsonlogic_decref(result);
```

To use the extra operators use `jsonlogic_apply_custom(logic, extras, &JsonLogic_Extras)`.
You can also define your own operations hash-table, but you need to include the
builtin operations for them to work:

```C
#include <math.h>

JsonLogic_Handle pow_op(void *context, JsonLogic_Handle data, JsonLogic_Handle args[], size_t argc) {
    if (argc < 2) {
        return JsonLogic_Error_IllegalArgument;
    }
    return jsonlogic_number_from(pow(jsonlogic_to_double(args[0]), jsonlogic_to_double(args[1])));
}

// ...

JsonLogic_Operations ops = JSONLOGIC_OPERATIONS_INIT;
error = jsonlogic_operations_extend(&ops, &JsonLogic_Builtins);
if (error != JSONLOGIC_ERROR_SUCCESS) {
    fprintf(stderr, "*** error: %s\n", jsonlogic_get_error_message(error));
    exit(1);
}

error = jsonlogic_operations_set(&ops, u"pow", NULL, pow_op);
if (error != JSONLOGIC_ERROR_SUCCESS) {
    fprintf(stderr, "*** error: %s\n", jsonlogic_get_error_message(error));
    exit(1);
}

result = jsonlogic_apply_custom(logic, data, &ops);
```

For CertLogic it is `certlogic_apply(logic, extras)` and
`certlogic_apply_custom(logic, extras, &CertLogic_Extras)`.

Remarks
-------

Locale dependant functions are used for printing numbers, and as such there
might be a `,` or anything else instead of the expected `.` in decimal numbers
if you set `LC_NUMERIC` to an according locale. To ensure valid JSON output
`LC_NUMERIC` needs to best be `C`. If you don't use `setlocale()` in your
program this shouldn't be a problem.

For parsing there are enough locale independant functions available on the
various operating systems for that to not be a problem, though.

This library's tagged pointer implementation uses the payload bits of 64 bit
floating-point NaNs, i.e. only strings, arrays, and objects are heap allocated,
numbers, booleans, null, and error codes are directly encoded in the NaN payload.
This requires pointers to never actually use more than 48 bits, which is true
for e.g. x86-64 and ARMv8. (And of course for any 32 bit architectures.) This
library is only tested on x86-64 (and i686), though. Maybe some more bit
manipulations are needed on ARMv8, in case the top 16 bits of pointers aren't
just all 0s there, I don't know.
