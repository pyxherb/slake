# Slake

<div style="text-align: center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
</div>

## Introduction

Slake is a statically-typed, general-purpose and embeddable
programming language for extensible applications.

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
  * Generic 🕓
    * Type-based Generic ✅
    * Value-based Generic ✅
    * Base Class and Interface Generic Constraint ✅
    * Trait Interface Generic Constraint 🕓
  * Function Invoking ✅
    * Regular Function ✅
    * Method Function 🕓
    * Closure Function 🕓
  * Function Type Representation 🕓
    * Regular Function 🕓
    * Method Function 🕓
    * Closure Function 🕓
  * Parameter-name-based Function Invoking 📝
  * Class Type 🕓
    * Instantiation ✅
    * Inheritance ✅
    * Cyclic Inheritance Checker ✅
  * Interface Type ✅
    * Implementing ✅
    * Cyclic Inheritance Checker ✅
  * Structure Type 🕓
    * Instantiation ✅
    * Copy Assignment ✅
    * Move Assignment 🕓
    * Type Recursion Checker ✅
  * Coroutine Type 🕓
    * Instantiation 🕓
    * Copy Assignment 🕓
    * Move Assignment 🕓
    * Type Recursion Checker 🕓
  * Exception Type 🕓
    * Detaching from Class Type 🕓
    * Throwing and Handling 🕓
  * Constant Enumeration (syntatic sugar) 🕓
  * Scoped Enumeration Type 🕓
  * Union Enumeration (ADT-like) Type 🕓
    * Instantiation 🕓
    * Copy Assignment 🕓
    * Move Assignment 🕓
    * Type Recursion Checker ✅
  * Union Enumeration Item Type 🕓
    * Instantiation 🕓
    * Copy Assignment 🕓
    * Move Assignment 🕓
    * Type Recursion Checker ✅
  * Tuple Type 🕓
    * Instantiation 🕓
    * Copy Assignment 🕓
    * Move Assignment 🕓
  * Value Type Boxing 🕓
    * Structure Type 🕓
    * Coroutine Type 🕓
    * Union Enumeration Type 🕓
    * Union Enumeration Item Type 🕓
    * Tuple Type 🕓
  * Parameter Pack 🕓
    * Unpacking as Parameters 🕓
  * Nullity Type Guard 🕓
    * Local Variable ✅
    * Non-local Variable 🕓
  * Conversion Type Guard 🕓
    * Local Variable 🕓
    * Non-local Variable 🕓
  * Pattern Matching 🕓
    * Exhaustion Check 🕓
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
  * Interpreter 🕓
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

To build Slake Compiler (slkc), a C++20 compiler and standard library is required.

## License

Slake is licensed under GNU Lesser Public License v3.0 with linking exception.
