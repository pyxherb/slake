.. SPDX-License-Identifier: GFDL-1.3-only OR CC-BY-SA-4.0

Frequently Asked Questions
==========================

I got segmentation faults when I accessing an object
----------------------------------------------------

The garbage collector does not check if an object was referenced by the host.
That means objects may be deleted after the host referenced.
