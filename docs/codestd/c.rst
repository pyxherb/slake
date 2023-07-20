.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

C Coding Standard
=================

.. contents::

Naming convention
-----------------

For private identifiers, add an `_` as a prefix.

Variable
~~~~~~~~

For most general variables, use `camelCase`.

.. code:: c

    int var;
    static int _privateVar;

    typedef struct _Struct {
        int pubMember;
        int _privateMember;
    } Struct;

Constants
~~~~~~~~~

For compile-time constants, use `UPPER_SNAKE_CASE`.

For runtime constants, use `camelCase`.

.. code:: C

    #include <time.h>

    #define CT_CONST1 1
    const int CT_CONST2 = 2;

    typedef enum _Enum {
        CT_CONST3 = 3,
        CT_CONST4
    } Enum;

    int main() {
        const int rtConst1 = 1;
        const int rtConst2 = rtConst1 + time(NULL);

        return rtConst2;
    }

Macros
~~~~~~

Generally, use `UPPER_SNAKE_CASE`.

Macros which imitate others should be treated like the imitate target.

.. code:: C

    // The macro does not imitate anything.
    #define COMPARATOR_DECL(name, type) void name(type a, type b)

    // The macro is imitating behavior of a constant
    #define CONN_MAX 64

    // Following macros imitate behaviors of functions
    #define min(a, b) ((a) < (b) ? a : b)
    #define max(a, b) ((a) > (b) ? a : b)

Functions
~~~~~~~~~

Generally, use `snake_case`.

The parameters should be treated as variables.

.. code:: C

    int this_is_a_function();
    char this_is_another_function(int param);

For methods, the name should be like:

`classname_methodname`

For example:

.. code:: C

    typedef struct _MyFile MyFile;

    MyFile* myfile_open(const char* path);
    size_t myfile_read(MyFile* fp, void* buf, size_t size);
    size_t myfile_write(MyFile* fp, const void* buf, size_t size);
    void myfile_seek(MyFile* fp, size_t off);
    size_t myfile_tell(MyFile* fp);
    void myfile_close(MyFile* fp);

Custom Type
~~~~~~~~~~~

Literal Types
`````````````

For literal types, use `snake_case` with an `_t` suffix.

.. code:: c

    typedef uint32_t addr_t;
    typedef uint16_t pid_t;

Structures/Unions/Enumerations
```````````````````````````````

.. warning::

    Do not add `_` as a prefix for any structure/union/enumeration even it is
    private.

For structures/unions/enumerations, use `UpperCamelCase`.

The type must have at least one type alias that allows the user using the type
without `struct`/`union`/`enum` prefix and the original type's name should has
an `_` as a prefix.

If the type is an enumeration and it will not be used as a type, it may not
need to have any alias for the, nor adding the `_` prefix.

.. code:: c

    #include <stdint.h>

    typedef enum _GenericValueType {
        VT_I8,
        VT_I16,
        VT_I32,
        VT_I64,
        VT_U8,
        VT_U16,
        VT_U32,
        VT_U64
        VT_F32,
        VT_F64
    } GenericValueType;

    typedef struct _GenericValue {
        union {
            int8_t i8;
            int16_t i16;
            int32_t i32;
            int64_t i64;
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
            float f32;
            double f64;
        } _data;
        GenericValueType type;
    } GenericValue;

Anonymous structures/unions/enumerations are not allowed.

.. code:: c

    // Not allowed
    typedef struct {
        int a;
    } Struct;

    // Not allowed, too
    typedef union {
        int i;
        float f;
    } Union;

    // Missing original enumeration name
    typedef enum {
        ITEM1,
        ITEM2,
        ITEM3
    } Enum;

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
