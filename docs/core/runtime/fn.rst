Function
========

Mangling
--------

Slake compiler will mangle name of all functions to support overloading and
generic.

For example, a function is declared as:

.. code:: slake

    void myFunc(i32 a, u32 b, @MyClass c)

and its name will be mangled into:

    myFunc$i32$u32$@MyClass

