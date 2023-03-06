Switch
======

.. contents::

Overview
--------

Switch statement allows you to match an expression continuously.

Syntax
------

.. code::

    switch (<Expression>) {
        case <Expression>: {
            [Statements]
        }
        case <Expression>: {
            [Statements]
        }
        //
        // ...
        //

        //
        // The`Default' branch is optional
        //
        default:
            [Statements]
    }

Example
-------

Following example will give corresponding message and its type.

.. code::

    string getMessage(int id) {
        switch (id) {
            case 0: { return "Hello, world!"; }
            case 1: { return "This is a test message."; }
            case 2: { return "I have delicious biscuits."; }
            case 3: { return "Your cookie has been expired."; }
        }
        return "Message not found";
    }

    string getMessageType(int id) {
        string type;
        switch(id) {
            case 0: {
                type = "This is not a test";
                // No break statement, will fall into the next case.
            }
            case 1: {
                type = "Test";
                break;
            }
            case 2: {
                type = "Response";
                break;
            }
            case 3: {
                type = "Warning";
                // No break statement, will fall into the default block.
            }
            default: {
                type = "Unknown";
            }
        }

        return type;
    }

    void main() {
        int id = 0;
        times(5) {
            std.io.println("Message #" + ((string)id) + " (" + getMessageType(id) + "): " + getMessage(id));
            id++;
        }
    }

Expected output::

    Message #0 (Test): Hello, world!
    Message #1 (Test): This is a test message.
    Message #2 (Response): I have delicious biscuits.
    Message #3 (Warning): Your cookie has been expired.
    Message #4 (Unknown): Message not found

Breaking & Fallthrough
----------------------

You can break the matching manually by adding break statements,
or fallthrough directly by using continue statements.

.. code::

    switch (expr) {
        case 1: {
            expr++;
        }
        // Because there is no break statement, the execution will fall into the next case.
        case 2: {
            expr++;
            break; // Will leave the whole switch statement.
        }
        case 3: {
            continue; // Will fall into the next case.
            break; // No effect
        }
        case 4: {
            expr--;
        }
    }
