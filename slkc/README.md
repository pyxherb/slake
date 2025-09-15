# About The Compiler

The compiler has 3 main modules:
* The AST framework
* The compilation framework (including semantic analysis)
* The language server (depends on the compilation framework)

## Issues

### Stack Overflow Risks on Everywhere

Several functions (such as AST node duplication, generic instantiation,
expression compilation, etc.) has risk of stack overflow, we have planned to
migrate to C++20 and use coroutine to avoid this problem.
