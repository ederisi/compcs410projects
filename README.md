# COMP.CS.410 projects

Build on Linux with GNU Make, a C++20 compiler, and development files for the running kernel:

```sh
make -j
```

This builds `launcher`, `dlopen`, `wsserver`, `encryption`, `libmathlib.so`, and
`libmathlib2.so` under `build/bin`. Object and dependency files also stay there,
under `build/bin/obj`.

Individual targets are available: `make launcher`, `make dlopen`,
`make wsserver`, `make encryption`, `make mathlib`, and `make mathlib2`.
The encryption executable is independent and is not registered in the launcher.

Run the existing launcher from its output directory so its relative executable
and shared-library paths resolve:

```sh
make run
```

## Cleanup

`make clean` removes `build/bin`, including all new build artifacts.
