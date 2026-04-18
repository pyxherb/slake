# About The Runtime

The runtime holds all created objects, manages memory allocation and releasing
and execute bytecodes by the interpreter.

## Solution of Interacting With The Native Side With Moving GC

A viable solution is that pin all of the objects involved when calling the native functions.

But the user still has to pin the object manually with the HostObjectRef.

## UB in Signed Integer Overflows

We have to check if an adding or multiplication will overflow and convert them
into exceptions to avoid UB.

## Removing Extraneous Allocator Parameter Of Object Constructors

Now we have many object types have extraneous allocator parameter after we
removed the `self_allocator` member, remove them.
