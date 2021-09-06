JsonLogic C Implentation
========================

Implementation for [JsonLogic](https://jsonlogic.com/) just for fun. No claims
for correctness or speed are made.

It includes a hand crafted JSON parser that is probably also quite slow compared
to other JSON parsers.

This library uses tagged pointers and ref counted garbage collecting, so if
you have any ref-loops you need to break them manually. Though the public API
only allows immutable use of JSON data and as such you can't craft ref-loops
with that anyway.

Extras and CertLogic
--------------------

The operations defined in `JsonLogic_Extras` (`jsonlogic_extras.h`) include the
operations needed to pass the [CertLogic](https://github.com/ehn-dcc-development/dgc-business-rules/tree/main/certlogic)
test suit (included in the tests of this library).

Limitations
-----------

This library's tagged pointer implementation uses the payload bits of 64 bit
floating-point NaNs, i.e. only strings, arrays, and objects are heap allocated,
numbers, booleans, null, and error codes are directly in the tagged pointer.
This requires pointers to never actually use more than 48 bits, which is true
for e.g. x86-64 and ARMv8. This library is only tested on x86-64, though. Maybe
some more bit manipulations are needed on ARMv8, in case the top 16 bits of
pointers aren't just all 0s there.

License
-------

Not sure yet. Maybe something like "LGPL, but allow static linking."
