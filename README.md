JsonLogic C Implentation
========================

Just for fun. Work in progress. No claims for correctness or speed are made.

Instead of a hashtable a sorted list with binary search for lookup is used, i.e. O(log n).
Maybe some day I'll use a hashtable, dunno.

It includes a hand crafted JSON parser that is probably also quite slow.

Also it uses ref counted garbage collecting, so if you have any ref-loops you need
to break them manually. If you only use objects coming from JSON you won't have
any ref-loops.
