Operators
=========

Introduction
------------

Operator is symbols/sequence of symbols that perform corresponding operation
with the operands.

Operator List
-------------

+---------+---------------+---------------------+--------------+
| Operator| Type          | Name                | Abbreviation |
+=========+===============+=====================+==============+
| ``!``   | Unary (Prefix)| Not                 | NOT          |
+---------+---------------+---------------------+--------------+
| ``-``   | Unary (Prefix)| Negate              | NEG          |
+---------+---------------+---------------------+--------------+
| ``~``   | Unary (Prefix)| Reverse             | REV          |
+---------+---------------+---------------------+--------------+
| ``++``  | Unary (Prefix)| Forward Increase    | INC_F        |
+---------+---------------+---------------------+--------------+
| ``++``  | Unary (Suffix)| Backward Increase   | INC_B        |
+---------+---------------+---------------------+--------------+
| ``--``  | Unary (Prefix)| Forward Decrease    | DEC_F        |
+---------+---------------+---------------------+--------------+
| ``--``  | Unary (Suffix)| Backward Decrease   | DEC_B        |
+---------+---------------+---------------------+--------------+
| ``+``   | Binary        | Add                 | ADD          |
+---------+---------------+---------------------+--------------+
| ``-``   | Binary        | Subtract            | SUB          |
+---------+---------------+---------------------+--------------+
| ``*``   | Binary        | Multiply            | MUL          |
+---------+---------------+---------------------+--------------+
| ``/``   | Binary        | Divide              | DIV          |
+---------+---------------+---------------------+--------------+
| ``%``   | Binary        | Remainder/Modulo    | MOD          |
+---------+---------------+---------------------+--------------+
| ``&``   | Binary        | And                 | AND          |
+---------+---------------+---------------------+--------------+
| ``|``   | Binary        | Or                  | OR           |
+---------+---------------+---------------------+--------------+
| ``^``   | Binary        | Exclusive Or        | XOR          |
+---------+---------------+---------------------+--------------+
| ``&&``  | Binary        | Logical And         | LAND         |
+---------+---------------+---------------------+--------------+
| ``||``  | Binary        | Logical Or          | LOR          |
+---------+---------------+---------------------+--------------+
| ``==``  | Binary        | Equal               | EQ           |
+---------+---------------+---------------------+--------------+
| ``===`` | Binary        | Strict Equal        | SEQ          |
+---------+---------------+---------------------+--------------+
| ``!=``  | Binary        | Inequal             | NEQ          |
+---------+---------------+---------------------+--------------+
| ``!==`` | Binary        | Strict Inequal      | SNEQ         |
+---------+---------------+---------------------+--------------+
| ``<<``  | Binary        | Left Shift          | LSH          |
+---------+---------------+---------------------+--------------+
| ``>>``  | Binary        | Right Shift         | RSH          |
+---------+---------------+---------------------+--------------+
| ``<``   | Binary        | Less Than           | LT           |
+---------+---------------+---------------------+--------------+
| ``>``   | Binary        | Greater Than        | GT           |
+---------+---------------+---------------------+--------------+
| ``<=``  | Binary        | Lesser Or Equal     | LEQ          |
+---------+---------------+---------------------+--------------+
| ``>=``  | Binary        | Greater Or Equal    | GEQ          |
+---------+---------------+---------------------+--------------+
| ``=``   | Binary        | Assign              | ASSIGN       |
+---------+---------------+---------------------+--------------+
| ``+=``  | Binary        | Plus Assign         | ADD_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``-=``  | Binary        | Subtract Assign     | SUB_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``*=``  | Binary        | Multiply Assign     | MUL_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``/=``  | Binary        | Divide Assign       | DIV_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``%=``  | Binary        | Remainder Assign    | MOD_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``&=``  | Binary        | And Assign          | AND_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``|=``  | Binary        | Or Assign           | OR_ASSIGN    |
+---------+---------------+---------------------+--------------+
| ``^=``  | Binary        | Exclusive Or Assign | XOR_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``<<=`` | Binary        | Left Shift Assign   | LSH_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``>>=`` | Binary        | Right Shift Assign  | RSH_ASSIGN   |
+---------+---------------+---------------------+--------------+
| ``<=>`` | Binary        | Swap/Exchange       | XCHG         |
+---------+---------------+---------------------+--------------+
| ``?:``  | Ternary       | Condition           | COND         |
+---------+---------------+---------------------+--------------+

Operator Behaviors for Built-in Types
-------------------------------------

All specializations and exceptions were noted.

Not
~~~

Returns `true` if the value is valid (not null, etc.), `false` otherwise.

i8/i16/i32/i64/u8/u16/u32/u64
'''''''''''''''''''''''''''''

Returns `true` if the value is zero, `false` otherwise.

f32/f64
'''''''

Returns `true` if the value is zero/NaN, `false` otherwise.

Negate
~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns negated value of the operand.

Reverse
~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns bitwise-reversed value of the operand.

Forward Increase
~~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
The operand will be added to 1 and the sum will be returned.

Backward Increase
~~~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
The operand will be added to 1 and the original value will be returned.

Forward Decrease
~~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
The operand will be subtracted with 1 and the sum will be returned.

Backward Decrease
~~~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
The operand will be subtracted with 1 and the original value will be returned.

Add
~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns sum of the operands.

Subtract
~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns difference of the operands.

Multiply
~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns product of the operands.

Divide
~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns quotient of the operands.

Remainder
~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns remainder of the operands.

And
~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns the result after the bitwise-and operation.

Or
~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns the result after the bitwise-or operation.

Exclusive Or
~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns the result after the exclusive or operation.

Logical And
~~~~~~~~~~~

bool
''''
Returns `true` if the operands are all `true`, `false` otherwise.

Logical Or
~~~~~~~~~~

bool
''''
Returns `true` if any operand is `true`, `false` otherwise.

Equal
~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the operands are equal, `false` otherwise.

Strict Equal
~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the operands are the same object, `false` otherwise.

Inequal
~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the operands are not equal, `false` otherwise.

Strict Inequal
~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the operands are not the same object, `false` otherwise.

Left Shift
~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns the result after the left-shift operation.

Right Shift
~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns the result after the right-shift operation.

Less Than
~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the left operand is lesser than the right operand, `false`
otherwise.

Greater Than
~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the left operand is greater than the right operand, `false`
otherwise.

Lesser Or Equal
~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the left operand is lesser than or equal to the right
operand, `false` otherwise.

Greater Or Equal
~~~~~~~~~~~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Returns `true` if the left operand is greater than or equal to the right
operand, false otherwise.

Assign
~~~~~~

i8/i16/i32/i64/u8/u16/u32/u64/f32/f64
'''''''''''''''''''''''''''''''''''''
Assign value of the right operand to the left operand.

Assign with Operation
~~~~~~~~~~~~~~~~~~~~~

Perform corresponding add operation and then the assign operation.

Swap
~~~~

Swap values of the operands.
The type of the operands must be the same.

Condition
~~~~~~~~~

Returns the second operand if the first operand is `true`, the third operand
otherwise.

Operator Overloading
--------------------

Slake allows you to override behaviors when executing operations with objects.

Overloadable Operators
----------------------

Unary
~~~~~

* ``!``
* ``-``
* ``~``
* ``++`` (Prefix)
* ``++`` (Suffix)
* ``--`` (Prefix)
* ``--`` (Suffix)

Binary
~~~~~~

* ``+``
* ``-``
* ``*``
* ``/``
* ``%``
* ``&``
* ``|``
* ``^``
* ``&&``
* ``||``
* ``==``
* ``!=``
* ``<<``
* ``>>``
* ``<``
* ``>``
* ``<=``
* ``>=``
* ``=``
* ``+=``
* ``-=``
* ``*=``
* ``/=``
* ``%=``
* ``&=``
* ``|=``
* ``^=``
* ``>>=``
* ``<<=``
* ``<=>``
