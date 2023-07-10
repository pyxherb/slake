.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

LOAD
====

Load value from a reference.

Usages
------

+-----------+-----------------------------------------------------------------+
| Operands  | Description                                                     |
+===========+=================================================================+
|           | Load a reference from stack and resolve it, then load resolved  |
|           | value.                                                          |
+-----------+-----------------------------------------------------------------+
| REF `ref` | Resolve `ref` and load resolved value.                          |
+-----------+-----------------------------------------------------------------+

Pseudo-Code
-----------

.. code::

    PROC load:
        CHECK_OPERAND_COUNT(0, 1)

        LET ref;

        IF nOperands == 1:
            ref = operands[0]
        ELSE
            ref = POP()
            IF VALUETYPE(ref) != VT_REF:
                THROW InvalidOperandsError
            END
        END

        LET value = RESOLVE(ref);

        IF value == NULL:
            THROW NotFoundError

        PUSH(value)
    END
