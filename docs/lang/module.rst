Module
======

.. contents::
.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Module Declaration
------------------

Module declaration declares scope name of current module.
Scope name of current module will be appended to all identifiers in the module
during the compilation.

For example:

.. code::

    module org::example;

    int main() {
        return 0;
    }

The function `main` will be renamed as `org::example::main`, the scope name
`org::example` will be appended to the function name `main`.

Module declaration is optional and the scope name of current module will be the
global scope if not set.

Module declaration can appear once only in a single module and must be the
first statement.
