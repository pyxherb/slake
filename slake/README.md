# About The Runtime

The runtime holds all created objects, manages memory allocation and releasing
and execute bytecodes by the interpreter.

## Issues

### Generic Instantiation Error

This error is in the example `hostext`.

This error occasionally occurs when we are loading the main module and
instantiating `AnotherGeneric`, the assertion in a generic argument replacement
operation, exactly at `type.getGenericArgTypeDef()` will occasionally fail for
unknown reason.
