# Hostext

A comprehensive test project used for testing the compiler and using the runtime as a host extension.

## Build

**NOTE: Before you continue, you have to compile the SLKC first.**

### CMake

After the configuration step, choose the `hostext` target and build or use command line:

```bash
cmake --build build --target hostext --config <Your config>
```

## Testing

Before running the `hostext`, you have to compile the host extension modules.

First, switch to the project's root directory (which contains the `slake` and the `slkc` directory).

In this case we have to compile `main.slk` and `extfns.slk`, use following command lines:

```bash
./build/slkc/slkc ./example/hostext/hostext/main.slk -o ./example/hostext/hostext/main.slx -I ./example/hostext/hostext/
./build/slkc/slkc ./example/hostext/hostext/hostext.slk -o ./example/hostext/hostext/hostext.slx -I ./example/hostext/hostext/
```

And then switch to the `example/hostext` directory in the project root and run the `hostext` executable in the build directory:

```bash
../../build/example/hostext/hostext
```
