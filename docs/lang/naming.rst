.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Naming
======

.. contents::

General Rules
=============

Rules listed here apply to all specific rules (unless otherwise noted). 

* For non-public members, add `_` as a prefix.

Specific Rules
==============

Enumerations
------------

* Generally, use `UpperCamelCase`
* Use `UPPER_SNAKE_CASE` for members

Modules
-------

* Use short `snakecase` or `snake_case`, short `snakecase` is preferred.
* DO NOT add `_` prefix for private members.

Variables
---------

* Generally, use `camelCase`.

Constants
---------

* Generally, use `UPPER_SNAKE_CASE`.

Literal Types
-------------

* Generally, use `snake_case` with `_t` suffix.

Classes
-------

* Generally, use `UpperCamelCase`.
* For abstract classes, add an `Abstract` as a prefix is recommended.

Interfaces
----------

* Generally, use `UpperCamelCase` with `I` prefix.

Traits
------

* Generally, use `UpperCamelCase` with `T` prefix.

Functions
---------

* Generally, use `camelCase`.
* For functions which check if a condition is true, add an `is` as a prefix is recommended.
* Parameters should be treated as variables.

Accessor
--------

* Generally, use `camelCase`

Generic Parameters
------------------

* Generally, use short `UpperCamelCase`, acronym is preferred.

Attributes
----------

* Generally, use `UpperCamelCase`.
* Parameters should be treated as variables.
