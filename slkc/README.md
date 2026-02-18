# About The Compiler

The compiler has 3 main modules:
* The AST framework
* The compilation framework (including semantic analysis)
* The language server (depends on the compilation framework)

## Issues

### Path-based Null Checker

Implement the path-based null checker.

```slake
let a : i32?;

a = 123;

// a is i32.

let b : i32?;

if (b === null) {
    // b is null.
} else {
    // optional, b is i32.
}
// b is perhaps i32?,

let b : i32?;

if (b === null) {
    // b is null.
    return;
}
// b is i32,
```

We have to be carefully to deal with following cases:

```slake
let a : i32?;

if (true) {
    a = 123;
} else {
    a = null;
}
// Then a should be i32.
```

```slake
let a : i32?;

while (true) {
    a = 123;
    if (true)
        break;
    a = null;
}
// Then a should be i32.
```

### Constructor Initialization Checker

Implement the constructor initialization checker.

Mostly in the same way of the path-based null checker.

Take notice of the initialization completion point, and switch the object type
using `DCMT` instruction on that time point.

### Cache Manager

We should introduce compilation cache for the language server.

This includes indexer/serializer/deserializer for memory objects.
