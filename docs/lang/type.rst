Type System
===========

.. contents::
.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Overview
--------

* Basic Type

  * `any`
  * `void` (null)
  * Arithmetic Types

    * Integral Types

      * Signed Types

        * `i8`
        * `i16`
        * `i32`
        * `i64`

      * Unsigned Types

        * `u8`
        * `u16`
        * `u32`
        * `u64`

    * Floating Point Types

      * `f32`
      * `f64`

    * `bool`

* Complex Type

  * Array Types
  * Map Types
  * Function/Closure Types
  * Custom Types

    * Class Types
    * Trait Types
    * Enumeration Types

Custom Type Name
----------------

Custom type name was represented as a reference with an `@`, for example:

.. code::

    @BasicUtils
    @std.core.except.IException
    @std.core.reflect.Module

are all valid custom type names.
