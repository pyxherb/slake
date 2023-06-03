Module
======

.. contents::
.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Module Declaration
------------------

Module declaration specifies name of current module.
Name of current module will be appended to name of all resources in the module
during the compilation.

For example:

.. code::

    module org::example;

    int main() {
        return 0;
    }

The function `main` will be renamed to `org::example::main`, the module name
`org::example` will be appended to the function name `main`.
