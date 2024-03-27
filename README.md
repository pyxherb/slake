# Slake

<div align="center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
    <big>The powerful object-oriented, type-safe programming language,</big>
    <big>designed for scripting and embedding.</big>
</div>

## Introduction

Slake is an object-oriented, type-safe programming language designed for
scripting and embedding.

## Development Progress

✅: Completed

🕓: Working In Progress

⏸: Paused/Postponed

❌: Cancelled

* Bytecode Compiler ✅
  * Lexer ✅
  * Parser ✅

* Core Language Compiler 🕓
  * Lexer ✅
  * Parser ✅
  * Compiler 🕓
  * Optimizer 🕓

* Language Server 🕓

* Language Standard 🕓
  * Control Flow ✅
  * Exception ✅
  * OOP Mechanism ✅
    * Class ✅
    * Interface ✅
    * Trait ✅
    * Operator ✅
    * Accessor ✅
  * Coroutine 🕓
  * Closure 🕓
  * Overloading ✅

* Runtime Library 🕓
  * Core Library (core) 🕓
    * Coroutine (coroutine) 🕓
    * Exceptions (except) 🕓
    * Reflection (reflect) 🕓
    * Traits (traits) 🕓
  * Standard Library (std) 🕓
    * I/O (io) 🕓
      * Stream (stream) 🕓
    * Mathematic Facilities (math) 🕓
      * RNG Facilities (rand) 🕓
    * Utilities (utils) 🕓
      * Concurrency (concurrent) 🕓
      * File System (fs) 🕓
      * Hashing (hash) 🕓
      * Iterator (iterator) 🕓
      * Range (range) 🕓
      * String Manipulating (str) 🕓

* Runtime 🕓
  * Built-in Compiler 🕓
  * Exception Mechanism ✅
  * Type Checker ✅
  * Interpreting ✅
  * Generic ✅
  * Overloading ✅
  * OOP Mechanism ✅
    * Class Instantiation ✅
    * Class Inheritance ✅
    * Interface Implementing ✅
    * Trait Compatibility Checker ✅
  * Closure 🕓
  * Coroutine ✅

## Building

To build Slake, you will always need:

* CMake (version > 3.13)
* A C++ compiler with C++17 support
* A C++17 standard library implementation

### Runtime

For x86 and x86-64, you will need:

* Netwise Assembler (NASM), or any other compatible assembler.

### Slake Compiler (slkc)

You will need:

* A C++ Compiler with C++17 support
* ANTLR 4, both jar executable and C++ runtime

### Bytecode Compiler (slkbc)

You will need:

* A C++ Compiler with C++17 support
* Flex
* GNU Bison
