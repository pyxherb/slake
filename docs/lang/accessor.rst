.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Accessor
========

.. contents::

Accessors behave like variables but is more flexible, you can control its
behavior like an instance of an anonymous class.

Syntax
------

An accessor requires a data type for itself like variables and corresponding
type conversion operator must be implemented.

Accessors have operator members only, and `this` cannot be referenced in the
context.

.. code::

    pub i32 _myInteger;
    pub i32 -> myInteger {
        // Required type conversion operator.
        operator i32() {
            return _myInteger;
        }

        // Optional type conversion operator, replaces its default type conversion behaviors.
        operator f32() {
            return (f32)_myInteger;
        }
        
        // Optional assignment operator.
        i32 operator =(i32 data) {
            _myInteger = data;
        }

        // Optional assignment operator.
        f32 operator =(f32 data) {
            _myInteger = (i32)data;
        }
    }

If an accessor has `any` as its data type, specific type conversion operator is
not required but the accessor must have at least one type conversion operator.

.. code::

    i32 _data;

    // Valid
    pub any -> validAccessor {
        operator i32() {
            return 0;
        }

        i32 operator =(i32 data) {
            _data = data;
        }
    }

    // Invalid, the accessor has no conversion operator
    pub any -> invalidAccessor {
        i32 operator =(i32 data) {
            _data = data;
        }
    }
