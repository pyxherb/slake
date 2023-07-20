# Slake

<div align="center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
    <big>Powerful object-oriented, type-safe programming language,</big>
    <big>for embedding into applications.</big>
</div>

## Introduction

Slake is an object-oriented, type-safe programming language which is designed
for embedding into applications.

## Development Progress

✅: Completed
🕓: Working In Progress
⏸: Paused/Postponed
❌: Cancelled

* Bytecode Compiler 🕓
  * Lexer ✅
  * Parser 🕓

* Core Compiler 🕓
  * Lexer ✅
  * Parser ✅
  * Compiler 🕓
  * Optimizer 🕓

* Language Server 🕓

* Core Language 🕓
  * Control Flow ✅
  * Exception ✅
  * OOP Mechanism 🕓
    * Class ✅
    * Interface ✅
    * Trait ✅
    * Operator ✅
    * Accessor ✅
  * Coroutine 🕓
  * Closure 🕓
  * Overloading ✅

* Runtime Library 🕓
  * Core (core) 🕓
    * Coroutine (task) 🕓
    * Exceptions (except) 🕓
    * Reflection (reflect) 🕓
    * Traits (traits) 🕓
  * Standard (std) 🕓
    * Operating System
      * File System (fs) 🕓
    * Utilities
      * Concurrency (concurrent) 🕓
      * Hashing (hash) 🕓
      * I/O (io) 🕓
      * Iterator (iterator) 🕓
      * Mathematic Facilities (math) 🕓
      * Random Number Generators (rand) 🕓
      * Range (range) 🕓
      * Ratio (ratio) 🕓

* Runtime 🕓
  * Built-in Compiler ❌
  * Exception Mechanism 🕓
  * Type Checker ✅
  * Interpreting ✅
  * Generic 🕓
  * Overloading ✅
  * OOP Mechanism 🕓
    * Class Instantiation ✅
    * Class Inheritance ✅
    * Interface Implementing ✅
    * Trait Compatibility Checker ✅
  * Closure 🕓
  * Coroutine 🕓

## Building

To build Slake, you will need:

* CMake (version > 3.13)
* Compiler with C++17 support
* An implementation of C++17 standard library

### Runtime

To build the runtime with Slake standard library, you may need some extra tools.

For x86 and x86-64, you will need:

* Netwise Assembler (NASM), or compatible assemblers.

### Slake Compiler (slkc)

You will need:

* Compiler with C++17 support
* ANTLR 4

Note that CMake cache variables for ANTLR4 may need to be configured manually.

### Bytecode Compiler (slkbc)

You will need:

* Compiler with C++17 support
* Flex
* GNU Bison
