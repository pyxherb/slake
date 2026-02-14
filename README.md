# Slake

<div style="text-align: center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
</div>

## Introduction

Slake is a statically-typed and embeddable programming language for extensible
applications.

It supports:

* Function Overloading
* Value-based Generics
* Union Enumeration (ADT-like)
* Coroutine
* and more...

## Development Progress

✅: Completed

🕓: Working In Progress

⏸: Paused/Postponed

📝: Planned

* Compiler 🕓
  * Type Checker 🕓
  * Nullable Detection 🕓
  * Type-based Generic ✅
  * Value-based Generic ✅
  * Function Invoking ✅
  * Parameter-name-based Function Invoking 📝
  * Class Instantiation ✅
  * Class Inheritance ✅
    * Cyclic Inheritance Checker ✅
  * Interface Implementing ✅
    * Cyclic Inheritance Checker ✅
  * Structure Instantiation 🕓
    * Type Recursion Checker ✅
  * Coroutine 🕓
    * As Type 🕓
  * Exception 🕓
  * Constant Enumeration (syntatic sugar) 🕓
  * Scoped Enumeration 🕓
  * Union Enumeration (ADT-like) 🕓
    * Type Recursion Checker ✅
  * Tuple Type 🕓
  * Parameter Pack 🕓
    * Unpacking as Parameters ✅
  * Type-specific Path (`with`) 🕓
  * Pattern Matching 🕓
  * Type-based Pattern Matching 🕓
  * Attribute 🕓
  * Lambda Expression 🕓
  * Generic Type Specialization 📝
  * Generic Function Specialization 📝
  * Macro 📝
  * Accessor (syntactic sugar, with back field accessing) 📝
* Language Server 📝
  * Linting 📝
  * Snippets 📝
  * Formatting 📝
  * Renaming 📝

* Runtime 🕓
  * Interpreter ✅ (Most of the functions are completed)
  * Program Analyzer 🕓
  * Program Optimizer 🕓
  * Runtime Library 🕓

* JIT Recompiler 🕓
  * x86-64 Backend 🕓
  * ARM64 Backend 📝
  * RISC-V Backend 📝

* AOT Compiler 🕓
  * Native C++ Backend (SLX2CXX) 🕓
  * WASM Backend (SLX2WASM) 📝

## Building

To build Slake, you will always need:

* CMake (version > 3.23)
* A C++ compiler with C++17 support
* A C++17 standard library, which at least has freestanding environment support.
* The PEFF library

## License

Slake is licensed under GNU Lesser Public License v3.0 with linking exception.
