Calling Convention
==================

.. contents::

Introduction
------------

Calling convention specifies details during calling functions.
This documentation describes the calling conventions used in Slake programs.

Conventions
-----------

stdstk
~~~~~~

**stdstk** is an acronym for **Standard Stack**.
This kind of calling convention is commonly used by most of functions.

The arguments will be pushed from the right to the left.

innercall
~~~~~~~~~

**innercall** is a kind of calling convention for internal functions (e.g. private functions).

syscall
~~~~~~~

**syscall** is used by system callings.
