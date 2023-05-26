.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

CAST
====

Convert a value to another type.

Usages
------

+-----------------+-----------------------------------------------------------+
| Operands        | Description                                               |
+=================+===========================================================+
| TYPENAME `type` | Pop a value and convert it to `type`, an exception will   |
|                 | be raised if the value cannot be converted.               |
+-----------------+-----------------------------------------------------------+
| ANY `value`,    | Convert `value` to `type`, an exception will be raised if |
| TYPENAME `type` | `value` cannot be converted.                              |
+-----------------+-----------------------------------------------------------+
