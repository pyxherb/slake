Interface
=========

.. contents::

Overview
--------

Interface is one of core concepts of Object-Orinted Programming (OOP) in Swampeak.

Syntax
------

.. code::

    <Access Modifier> interface <Name> : [Parent Interfaces] {
        <Methods>
    }

Example
-------

Consider a person wants to send a mail to another one via a carrier:

.. code::

    import {
        std = @"std"
    }
    pub interface IOrigin {
        fn send(@IReceiver& destination);
    }

    pub interface IDestination {
        fn receive(int information);
    }

    pub interface IMedium : @IOrigin, @IDestination {
    }

    pub class Sender : @IOrigin {
        fn send(@IReceiver& destination) {
            destination.receive(information, 123456);
        }
    }

    pub class Receiver : @IDestination {
        pub int information;

        fn receive(int information) {
            self.information = information;
        }
    }

    pub class Carrier : @IMedium {
        int information;

        fn send(@IReceiver& destination) {
            destination.receive(information, information);
        }
        fn receive(int information) {
            self.information = information;
        }
    }

    pub fn main() {
        @IOrigin sender = new @Sender();
        @IDestination receiver = new @Receiver();
        @IMedium carrier = new @Carrier();

        sender::send(carrier);
        carrier::send(receiver);

        std::print(receiver::information);
    }

In this example, we use 2 interfaces: ``IOrigin`` and ``IDestination`` to
represent classes that have a particular trait/behavior. ``IOrigin`` is an
interface for classes that can send information to others, and
``IDestination`` is an interface for classes that can receive information from
others.

As you see, the sender is an origin and the receiver is a destination.
Because the carrier can receive information but can also transmit,
the carrier is an origin (transmitter), is also a destination (receiver).

The example should output::

    123456