# About The Compiler

The compiler has 3 main modules:
* The AST framework
* The compilation framework (including semantic analysis)
* The language server (depends on the compilation framework)

## Issues

### SLKC_RETURN_IF_XXX trap

In some cases you may see:

```cpp
if(/* condition */)
    SLKC_RETURN_IF_XXX(/*...*/);
else
    /* Do something... */;
```

does not pass the compilation, that is because expansion of `SLKC_RETURN_IF_XXX` by add a pair of braces like following:

```cpp
if(/* condition */) {
    SLKC_RETURN_IF_XXX(/*...*/);
} else
    /* Do something... */;
```

### Stack Overflow In Generic Instantiator

There's a stack overflow risk in the generic instantiator, perhaps we can
replace it with iteration method instead of recursion?
