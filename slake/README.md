# About The Runtime

The runtime holds all created objects, manages memory allocation and releasing
and execute bytecodes by the interpreter.

## Solution of Interacting With The Native Side With Moving GC

A viable solution is that pin all of the objects involved when calling the native functions.

But the user still has to pin the object manually with the HostObjectRef.

## UB in signed integer overflows

We have to check if an adding or multiplication will overflow and convert them
into exceptions to avoid UB.
