.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Module
======

.. contents::

Module Declaration
------------------

Module declaration declares name of current module.
Scope name of current module will be appended to all identifiers in the module
during the compilation.

For example:

.. code::

    module org::example;

    int main() {
        return 0;
    }

The function `main` will be renamed as `org::example::main`, the module name
`org::example` will be appended to the function name `main`.

Module declaration is optional, if the module name was not set, the name will
be set as empty (global scope for the runtime).

Module declaration can appear once only in a single module and must be the
first statement.

If parent scope of a module does not present, the runtime will create a module
object as the parent.
