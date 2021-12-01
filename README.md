JsonLogic C Implentation
========================

Implementation of [JsonLogic](https://jsonlogic.com/) and
[CertLogic](https://github.com/ehn-dcc-development/dgc-business-rules/tree/main/certlogic/specification)
just for fun. No claims for correctness or speed are made.

It includes a hand crafted JSON parser that is probably also quite slow compared
to other JSON parsers.

This library also implements CertLogic, which is a JsonLogic dialect. CertLogic
mainly removes a lot of operations, adds others, and makes empty objects falsy
(which are truthy under JsonLogic).

In addition to that this library provides some optional [extra operations](#extras)
in `jsonlogic_extras.h`.

This library uses tagged pointers and ref counted garbage collecting, so if
you have any ref-loops you need to break them manually. Though the public API
only allows immutable use of JSON data and as such you can't craft ref-loops
with that anyway.

I've also written a JsonLogic+CertLogic implementation in Python:
[panzi-json-logic](https://github.com/panzi/panzi-json-logic)

* [Usage Example](#usage-example)
* [Build](#build)
  * [Build Release](#build-release)
  * [Cross Compilation](#cross-compilation)
* [Extras](#extras)
* [Remarks](#remarks)

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

// Run JsonLogic expression:
JsonLogic_Handle result = jsonlogic_apply(logic, data);

// Prints: "true\n"
jsonlogic_println(stdout, result);

// Free memory:
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

For CertLogic it is `certlogic_apply(logic, data)` and
`certlogic_apply_custom(logic, data, &CertLogic_Extras)`.

Build
-----

This library has zero external dependencies (hence slow JSON parsing).

Make static library:

```bash
make static
```

This generates these assets:

* `build/$target/$release/include/jsonlogic.h`
* `build/$target/$release/include/jsonlogic_extras.h`
* `build/$target/$release/include/jsonlogic_defs.h`
* `build/$target/$release/lib/libjsonlogic.a`

`$target` may be a string like `linux64` or `darwin32` or similar. Per default
it is:

```bash
echo $(uname|tr '[:upper:]' '[:lower:]')$(getconf LONG_BIT)
```

For more see [Cross Compilation](#cross-compilation) below.

`$release` may be `debug` or `release`. (default: `debug`) For more see
[Build Release](#build-release) below.

Make shared library:

```bash
make shared
```

This generates these assets:

* `build/$target/$release/include/jsonlogic.h`
* `build/$target/$release/include/jsonlogic_extras.h`
* `build/$target/$release/include/jsonlogic_defs.h`
* `build/$target/$release/lib/libjsonlogic.so`

Make examples:

```bash
make examples
make examples_shared
```

Run tests:

```bash
make test
make test_shared
make valgrind
```

### Build Release

To build any of the targets from above in release mode pass `RELEASE=ON`:

```bash
make static RELEASE=ON
```

Per default it will build in debug mode using no optimizations and producing
binaries that include debug symbols.

### Cross Compilation

You can cross-compile for 32 bit/64 bit under 64 bit/32 bit and you can cross
compile for Windows under Linux using mingw:

```bash
make TARGET=linux32
make TARGET=linux64
make TARGET=win32
make TARGET=win64
make TARGET=darwin32
make TARGET=darwin64
```

Extras
------

This library also includes some extra operators that are not part of JsonLogic.
These are defined in the `JsonLogic_Extras` hash-table. This hash-table already
includes `JsonLogic_Builtins`. The same extras but combined with
`CertLogic_Builtins` is defined as `CertLogic_Extras`. The CertLogic extras also
include all the operations from JsonLogic that are otherwise missing from
CertLogic, but with CertLogic semantics for `!` and `!!` (i.e. empty objects are
falsy in CertLogic, but truthy in JsonLogic).

```C
#include <jsonlogic_extras.h>

// ...

result = jsonlogic_apply_custom(logic, data, &JsonLogic_Extras);
```

### `now`

Retrieve current time as milliseconds since the Unic epoch (1970-01-01 UTC).

```
{
    "now": []
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"now":[]}' null
1631412713602
```

### `timestamp`

Parse RFC 3339 date and date-time strings. Date-time strings without an explicit
time zone offset are assumed to be in UTC. Note that RFC 3339 is a subset of
ISO 8601, but it is actually what most people think of when they think of ISO
date-time strings.

Numbers will be passed through.

```
{
    "timestamp": [
        <string-or-number>
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"timestamp":"2022-01-02"}' null
1641081600000
$ ./build/linux64/debug/examples/jsonlogic_extras '{"timestamp":"2022-01-02T15:00:00+02:00"}' null
1641128400000
```

**NOTE:**

You need to use `timestamp` before comparing timestamps with date-times
provided as a string or you'll get wrong results. Assume the current time is
somewhen in 2021:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"<": [{"now":[]}, "2022-01-02"]}' null
false
$ ./build/linux64/debug/examples/jsonlogic_extras '{"<": [{"now":[]}, {"timestamp":"2022-01-02"}]}' null
true
```

However CertLogic has operators that are doing that for you:

```bash
$ ./build/linux64/debug/examples/certlogic_extras '{"before": [{"now":[]}, "2022-01-02"]}' null
true
```

### `format-time`

Format a timestamp (or date-time encoded as a string) as a RFC 3339 string.

```
{
    "format-time": [
        <string-or-number>
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"format-time":{"now":[]}}' null
"2021-09-12T02:21:26.732+00:00"
```

### `time-since`

Milliseconds since given date-time.

```
{
    "time-since": [
        <string-or-number>
    ]
}
```

Exmaple:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"time-since":"2021-01-02T15:00:00+02:00"}' null
21820927164
```

### `hours`

Convert hours to milliseconds. Useful in combination with `time-since`.

```
{
    "hours": [
        <number>
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"hours": 2}' null
7200000
```

### `days`

Convert days to milliseconds. Useful in combination with `time-since`.

```
{
    "hours": [
        <number>
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"days": 2}' null
172800000
```

### `combinations`

Return array of arrays that represent all combinations of the elements of all
the lists.

```
{
    "combinations": [
        <array>...
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"combinations": [[1, 2],["a", "b"]]}' null
[[1,"a"],[1,"b"],[2,"a"],[2,"b"]]
```

### `to-array`

Converts the argument into an array, if possible.

If the argument is already an array it is return as-is. If the argument is a
string then an array of strings returned, where the elements represent the
UTF-16 code units of the string. If the argument is an object then an array of
it's keys is returned. For any other argument an illegal argument error is
returned.

```
{
    "to-array": [
        <array-or-string-or-object>
    ]
}
```

### `zip`

Turns an array of array representing columns into an array of array representing
rows. The resulting array will be as long as the shortest column-array.

```
{
    "zip": [
        <array>...
    ]
}
```

Example:

```bash
$ ./build/linux64/debug/examples/jsonlogic_extras '{"zip": [[1, 2],["a", "b"]]}' null
[[1,"a"],[2,"b"]]
```

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
