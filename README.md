# Slake

<div style="text-align: center">
    <img src="./common/logo2.svg" alt="Slake logo"><br/>
</div>

## Introduction

Slake is a type-safe and embeddable programming language for extensible
applications.

## Development Progress

✅: Completed

🕓: Working In Progress

⏸: Paused/Postponed

📝: Planned

* Bytecode Compiler 📝

* Core Language Compiler 🕓
  * Lexer ✅
  * Parser ✅
  * Compiler ✅
  * Language Server 🕓
    * Linting 🕓
    * Snippets 🕓
    * Formatting 🕓
    * Renaming 🕓

* Runtime 🕓
  * Exception Mechanism ✅
  * Type Checker ✅
  * Interpreting ✅
  * Generic ✅
  * Overloading ✅
  * OOP Mechanism ✅
    * Class Instantiation ✅
    * Class Inheritance ✅
    * Interface Implementing ✅
  * Coroutine 🕓
  * Optimizer 🕓
    * Program Analyzer 🕓
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

### Runtime

For x86 and x86-64, you will need:

* Netwise Assembler (NASM), or any other compatible assembler.

### Slake Compiler (slkc)

You will need:

* A C++ Compiler with C++20 support
