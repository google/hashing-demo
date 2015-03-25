#Introduction

This is a sample implementation of a forthcoming C++ standard library
proposal. It is intended only for demonstration and evaluation purposes,
and is not suitable for use in production systems.

The APIs in std.h and std_impl.h are proposed for standardization. 
fnv1a.h and farmhash.h are example implementations of particular algorithms
using this framework, but are not themselves proposed for standardization.

#User-facing API

The user-facing API for hashing a value `v` using a hash algorithm `H` looks
like this:

```
typename H::result_type result = std::hash_value<H>(v);
```

The `H` parameter is defaulted, so this can be called more simply if you
don't care about the hash algorithm:

```
size_t result = std::hash_value(v);
```

There is also a functor interface akin to `std::hash`:

```
std::hasher h1;
size_t result1 = h1(v);
std::hasher<H> h2;
typename H::result_type result2 = h2(v);
```

These are all wrappers around the following basic interface:

```
typename H::state_type state;
auto result = typename H::result_type(hash_combine(H(&state), v));
```

`v`'s type must model the `Hashable` concept, which specifies the requirements
a type must satisfy in order to compute its hash value. H must model the
`HashCode` concept, which specifies how hash algorithms are expressed in
this framework.

#Hash algorithm requirements

A type `H` models the `HashCode` concept if the following expressions are
well-formed, and have the semantics described below, where `h` is an rvalue of
type `H`:

Expression                    Notes
----                          ----
`H::result_type`                Names a type
`H::state_type`                 Names a type
`H(&s)`                         `s` is an lvalue of type `H::state_type`.
`H(h)`
`hash_combine(h, vs...)`        Returns an rvalue of type `H`. `vs...` represents
                              an arbitrary number of `Hashable` arguments
`hash_combine_range(h, i, j)`   Returns an rvalue of type `H`. `i` and `j` are
                              `InputIterator`s that form a valid range, whose
                              `value_type` is `Hashable`.
`typename H::result_type(h)`

Values of a type `H` that models the `HashCode` concept represent intermediate
states in the computation of the hash value. As such, they also implicitly
represent the final hash value which would be derived from that state (hence
the name `HashCode`). That final hash value can be obtained by converting the
`H` value to `H::result_type`. `hash_combine` and `hash_combine_range` combine
an existing state with some input values to produce a new state.

If two `H` values are equivalent, they must yield equal values when converted
to `H::result_type`. If they are not equivalent, then with high probability
they must yield non-equal values when converted to `H::result_type`.
"Equivalence" between `H` values is an equivalence relation (reflexive,
symmetric, and transitive), and is defined inductively as follows:

* `H(&s1)` is equivalent to `H(&s2)`, where `s1` and `s2` are
  default-constructed `state_type` objects.
* If `h1` and `h2` are rvalues of type `H`, and `h1` is equivalent to `h2`,
  then:
  * `H(h1)` is equivalent to `H(h2)`.
  * `hash_combine(h1)` is equivalent to `h2`.
  * If `hash_combine(h1, vs...)` is equivalent to `hash_combine(h2, ws...)`,
    then `hash_combine(h1, v, vs...)` is equivalent to 
    `hash_combine(hash_combine(h2, w), ws...)`, where `v` and `w` have the same
    type and are equal.
  * If `i1 == i2`, then `hash_combine_range(h1, i1, i2)` is equivalent to `h2`.
  * If `i1 < i2`, `j1 < j2`, and `hash_combine_range(h1, next(i1), i2)` is
    equivalent to `hash_combine_range(h2, next(j1), j2)`, then
    `hash_combine_range(h1, i1, i2)` is equivalent to
    `hash_combine_range(hash_combine(h2, *j1), next(j1), j2)`,
    where `*i1` and `*j1` have the same type and are equal.

#Hashable type requirements

A type `T` models the `Hashable` concept if the expression
`hash_decompose(h, t)` is well-formed for any rvalue `t` of type `T` and
any object `h` whose type `H` is a model of `HashCode`, and that expression
yields an rvalue of type `H`. If `h1` and `h2` are equivalent rvalues of
type `H`, and `t1` and `t2` are possibly-const rvalues of type `T`, then
`hash_decompose(h1, t1)` and `hash_decompose(h2, t2)` must be equivalent if
`t1 == t2`, and must not be equivalent otherwise. To avoid infinite
recursion, the evaluation of `hash_decompose(h, t)` must not directly or
indirectly invoke `hash_decompose(hh, t)`, where `hh` is any value of type `H`.

#Limiting recursion

These requirements imply that `hash_decompose` must be implemented in terms of
`hash_combine` and `hash_combine_range` (except in the degenerate case that
all values of `T` are equal). By the same token, in the general case
`hash_combine` and `hash_combine_range` must be implemented in terms of
`hash_decompose`. However, they also supply the base case for the recursion:
`hash_combine` and `hash_combine_range` must not invoke `hash_decompose` on
`unsigned char`, which is the type of raw bytes and therefore the 'most
primitive' type. They may also choose to treat other types as primitive, as
an optimization.

If they do call `hash_decompose`, `hash_combine` and `hash_combine_range`
must only call `hash_decompose` directly on the input values; they may not
transform the inputs in any way (e.g. manually decomposing them or combining
them). This enables `hash_decompose` to avoid infinite recursion, since it
can predict what types `hash_decompose` will be recursively called on. Thus,
`hash_decompose` for type `T` must invoke `hash_combine`/`hash_combine_range`
on types that are in some sense more primitive than `T` (as the name suggests).

#Uniquely represented types

When the data to be hashed is contiguous in memory, it will be more efficient
to hash the entire memory region at once, avoiding the iteration and recursion
overhead of reducing all the way to `unsigned char`. However, this optimization
cannot always be applied. For example, in most implementations a `double[]`
cannot be hashed by hashing the underlying bytes of the array, because
IEEE double has distinct byte representations for `-0` and `+0`, but those
values compare equal, so they must have the same hash value.

In general, it is safe to hash a `T` value by hashing its bytes only if `T` is
*uniquely represented*, which means that if two `T` values have a different
byte representation, they will not compare equal. To enable hash algorithms
to apply this optimization safely, we propose an `is_uniquely_represented<T>`
trait to detect this property. The specializations for built-in and
library types should be sufficient for most cases, but users can also
specialize it for their types, if necessary. Note that
`is_uniquely_represented<T>` is always false unless explicitly specialized;
the compiler cannot reasonably deduce a correct value since it's a
high-level semantic property. `is_uniquely_represented<unsigned char>` is
required to be true, but is implementation-defined for all other
standard types (unless there's existing wording to support that guarantee).

#Heterogeneous lookup

In order to support heterogeneous lookup in hash containers, we will need
hash functions that can guarantee to produce equal hash values for equal
input values, even if the input values have different types. This is
impossible in general, but it can be achieved within restricted families
of related types, such as string-like types and pointer-like types
(note that integral types do not form such a family, because `==` is not
transitive within integral types).

Achieving this property requires cooperation between the hash algorithm
and the types being hashed. `hash_decompose` must be written so as to provide
the aforementioned guarantee, and `hash_combine`/`hash_combine_range` must
ensure that the recursion reaches that `hash_decompose` call. The optimizations
described above for uniquely-represented types cannot be applied in
this case, because even if the types are uniquely represented, particular
values may not have a unique representation across types. Consequently,
only some `HashCode` types can support heterogeneous lookup; they will
have to be marked with a trait, similarly to `is_transparent` for comparators.

This example code does not yet have an end-to-end demonstration of
heterogeneous lookup, but it does demonstrate type-invariant and
non-type-invariant versions of the FNV-1A algorithm.
