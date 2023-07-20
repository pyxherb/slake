.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

C++ Coding Standard
===================

.. contents::

Naming convention
-----------------

For private identifiers, add an `_` as a prefix.

Variable
~~~~~~~~

For most general variables, use `camelCase`.

.. code:: cpp

    int var;
    static int _privateVar;

    class Class {
    public:
        int pubMember;

    private:
        int _privateMember;
    }

Constants
~~~~~~~~~

For compile-time constants, use `UPPER_SNAKE_CASE`.

Constants which initialize at runtime should be treated as variables.

.. code:: cpp

    #include <ctime>
    #include <cstdint>

    #define MACRO_CONST 1
    const int VAR_CONST = 2;
    constexpr static int CONSTEXPR_CONST = 3;

    enum class _Enum : uint8_t {
        ENUMCONST1 = 3,
        ENUMCONST2
    } Enum;

    int main() {
        const int varConst1 = 1;
        const int varConst2 = rtConst1 + time(NULL);

        return rtConst2;
    }

Macros
~~~~~~

Generally, use `UPPER_SNAKE_CASE`.

Macros which imitate others should be treated like the imitate target.

.. code:: cpp

    // The macro does not imitate anything.
    #define COMPARATOR_DECL(name, type) void name(type a, type b)

    // The macro is imitating behavior of a constant
    #define CONN_MAX 64

    // Following macros imitate behaviors of functions
    #define min(a, b) ((a) < (b) ? a : b)
    #define max(a, b) ((a) > (b) ? a : b)

Functions/Methods
~~~~~~~~~~~~~~~~~

Generally, use `camelCase`.

The parameters should be treated as variables.

.. code:: cpp

    int thisIsAFunction();
    char thisIsAnotherFunction(int param);

Custom Type
~~~~~~~~~~~

Use `UpperCamelCase`.

For aliases, use `using` statements instead of `typedef` statements.

Do not use `typedef` for structures/unions/enumerations.

.. code:: cpp

    typedef unsigned int MyFlags;
    
    struct Struct {
        int member1;
        char member2;
        float member3;
    }

    class Class {
    protected:
        int protectedMember = 0;

    private:
        float _privateMember = 0.0f;

    public:
        Class() = default;
        virtual ~Class() = default;
    }

Namespaces
~~~~~~~~~~

Use `snake_case`.

Program Behaviors
-----------------

If a bug is from upstream components (dependencies), it should be reported to
maintainers of the components and await for fix up, or replace with other
replacements.

Error Handling
~~~~~~~~~~~~~~

All error handling mechanisms are welcome, except:

* Platform-specific mechanisms, e.g. SEH
* Exception-styled mechanisms, e.g. C++ exceptions, setjmp & longjmp

Memory Management
~~~~~~~~~~~~~~~~~

Memory Leaks
````````````

Memory leaks are not allowed and should be fixed up in principle.

Memory leaks in libraries are absolutely not allowed (even they were upstreams)
and should be fixed up before release versions.

Memory leaks in executables are unacceptable and should be fixed up. If the
leaks were from upstream components, presence of the bug may be allowed in
releases if the bug is not serious (eg. size of leaks does not grow).

Inline Functions/Methods
~~~~~~~~~~~~~~~~~~~~~~~~

All generic functions/methods (including methods in generic structures/classes)
should be inlined.

.. code:: cpp

    template <typename T>
    struct GenericStruct {
        T a;

        inline T myMethod() {
            return a;
        }
    };

    template <typename T>
    class GenericClass {
    protected:
        T a;

    public:
        inline T myMethod() {
            return a;
        }

        template <typename T1>
        inline T1 myGenericMethod() {
            return (T1)a;
        }
    };

Inlining virtual methods are not recommended.
