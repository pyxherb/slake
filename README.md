# Slake

<div align="center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
    <big>The object-oriented, type-safe, embeddable programming language</big>
</div>

## Introduction

Slake is an object-oriented, type-safe and embeddable programming language for
scripting.

## Development Progress

✅: Completed

🕓: Working In Progress

⏸: Paused/Postponed

❌: Cancelled

* Bytecode Compiler ⏸
  * Lexer ⏸
  * Parser ⏸

* Core Language Compiler 🕓
  * Lexer ✅
  * Parser ✅
  * Compiler ✅
  * Optimizer ❌
  * Language Server 🕓
    * Linting ✅
    * Snippets 🕓
    * Formatting 🕓
    * Renaming 🕓

* Language Standard 🕓
  * Control Flow ✅
  * Exception ✅
  * OOP Mechanism ✅
    * Class ✅
    * Interface ✅
    * Trait ❌
    * Operator ✅
    * Accessor ❌
  * Coroutine 🕓
  * Closure ❌
  * Overloading ✅

* Runtime Library 🕓
  * Core Library (core) 🕓
    * Coroutine (coroutine) 🕓
    * Exceptions (except) 🕓
    * Reflection (reflect) 🕓
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
  * Built-in Compiler ❌
  * Exception Mechanism ✅
  * Type Checker ✅
  * Interpreting ✅
  * Generic ✅
  * Overloading ✅
  * OOP Mechanism ✅
    * Class Instantiation ✅
    * Class Inheritance ✅
    * Interface Implementing ✅
  * Closure ❌
  * Coroutine 🕓

## Building

To build Slake, you will always need:

* CMake (version > 3.23)
* A C++ compiler with C++17 support
* A C++17 STL

### Runtime

For x86 and x86-64, you will need:

* Netwise Assembler (NASM), or any other compatible assembler.

### Slake Compiler (slkc)

You will need:

* A C++ Compiler with C++17 support
* jsoncpp library (If with language server support enabled)
