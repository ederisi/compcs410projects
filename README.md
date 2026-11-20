# COMP.CS.410 projects

This repository houses a collection of low-level systems programming and C++ projects developed for COMP.CS.410, focusing on dynamic library loading, networking protocols, Linux kernel module development, and process orchestration.

## Project Structure

The project is organized into modular directories under a unified build system managed by a root `Makefile`:

```text
.
├── 42-dlopen/                  # Dynamic math expression evaluator & script executor
│   ├── calculatorUtils.h       # Two-stack shunting/evaluation algorithms
│   ├── dlopen.h / .cpp         # Runtime dynamic library loading layer (dlopen/dlsym)
│   ├── mathlib.cpp             # Binary math operations shared library (.so)
│   ├── mathlib2.cpp            # Unary math operations shared library (.so)
│   ├── main.cpp                # CLI interactive expression parser & script runner
│   └── Makefile                # Sub-makefile delegating to root
│
├── 74-wsserver/                # Standalone C++ WebSocket server
│   ├── websocket.h / .cpp      # Handshake parsing, manual SHA-1 & Base64 encoding
│   ├── main.cpp                # TCP socket server, daemonization, & time broadcaster
│   └── Makefile                # Sub-makefile delegating to root
│
├── 82-encryption/              # Linux kernel module & user-space encryption tool
│   ├── sd.h                    # Shared kernel/user IOCTL definitions & data structures
│   ├── sd.c                    # Character device driver (/dev/secret, major 42)
│   ├── crypto.c                # Kernel-space XOR, custom SHA-1, & Base64 routines
│   ├── main.cpp                # CLI utility for key configuration and file encryption
│   └── Makefile                # Sub-makefile delegating to root
│
├── processhelpers.h            # Fork, process group, signal handling, & process reaping
├── main.cpp                    # Interactive terminal launcher CLI (run, status, kill)
├── Makefile                    # Centralized root build system
└── README.md                   # Project documentation and setup guide
```

## Component breakdown

### 42-dlopen

Evaluates complex arithmetic expressions using runtime-loaded shared libraries (libmathlib.so, libmathlib2.so) without static linking.

### 74-wsserver

Lightweight TCP WebSocket server that streams periodic time frames to connected clients. Includes hand-rolled SHA-1 and Base64 implementations.

### 82-encryption

Linux kernel character device driver (/dev/secret) implementing a cryptographic pipeline: XOR Cipher $\rightarrow$ SHA-1 Hash $\rightarrow$ Base64 Encoding.

## Build Compilation

Service manager and launcher utility for orchestrating background services (wsserver) and interactive CLIs.

Build on Linux with GNU Make, a C++20 compiler, and development files for the running kernel:

```sh
make -j
```

This builds `launcher`, `dlopen`, `wsserver`, `encryption`, `libmathlib.so`, and
`libmathlib2.so` under `build/bin`. Object and dependency files also stay there,
under `build/bin/obj`.

Individual targets are available: `make launcher`, `make dlopen`,
`make wsserver`, `make encryption`, `make mathlib`, and `make mathlib2`.

## Running the Programs (dlopen, wsserver, and encryption)

Run the Make commands from the project root. The run targets start the programs from `build/bin` so relative paths to shared libraries, scripts, and logs resolve there.

Start the launcher with `make run`. Use `sudo make run` if the encryption client needs elevated privileges to access `/dev/secret`.

### 42-dlopen (Math Expression & Script Evaluator)

Run directly:

```sh
make run-dlopen
```

Or at the launcher prompt:

```text
launcher> run dlopen
```

At the calculator prompt (`Enter your input:`), type an expression or enter a `.txt` script path:

- Arithmetic and power: `2 + 3 * 4`, `(10 - 4) / 2`, `2 ^ 8`
- Unary math functions: `sqrt(16)`, `sin(0)`, `cos(0)`, `tan(0)`
- Script execution: enter a path such as `commands.txt`. Relative paths start in `build/bin`.

Press Ctrl+C to exit.

### 74-wsserver (WebSocket Time Server)

The server listens on port 8080 (`0.0.0.0:8080`) and sends the current system time as a WebSocket text frame every 3 seconds after a client completes the RFC 6455 handshake.

Run in the foreground with logs on stdout and stderr. Press Ctrl+C to stop it.

```sh
make run-wsserver
```

Redirect stdout and stderr to `build/bin/ws_server.log` with `-b`:

```sh
make run-wsserver ARGS=-b
```

To manage it as a background service, use the launcher:

```text
launcher> run wsserver
launcher> status
launcher> kill wsserver
```

### 82-encryption (`/dev/secret` Kernel Driver Client)

Load the `secret_driver` kernel module first. See [Building the encryption driver on Linux](#building-the-encryption-driver-on-linux).

Set the XOR key (a 32-bit integer stored in the driver), then process one or more messages. The driver applies XOR, SHA-1, and Base64 in that order, and the client prints the results.

```sh
sudo make run-encryption ARGS='--setkey 42'
sudo make run-encryption ARGS='--encrypt "first message" "second message"'
```

Or at the launcher prompt:

```text
launcher> run encryption --setkey 42
launcher> run encryption --encrypt "first message" "second message"
```

## Cleanup

`make clean` removes `build/bin`, including all new build artifacts.

# Building the encryption driver on Linux

These steps target Linux systems with GNU make, a C/C++ toolchain, kernel build headers, and a kernel that supports external modules. Secure Boot signing steps use MOK, available on many shim-based distributions. Other systems may use a different key-enrollment process.

## 1. Install build requirements

Install your distribution's equivalents of:

- GNU make, GCC, and G++

- OpenSSL

- Kernel headers/development files matching the running kernel

- mokutil, if using MOK enrollment

Package examples:

```bash

# Fedora

sudo dnf install make gcc gcc-c++ openssl mokutil kernel-devel

# Debian or Ubuntu

sudo apt install build-essential libssl-dev mokutil linux-headers-$(uname -r)

# Arch Linux

sudo pacman -S base-devel openssl mokutil linux-headers

```

Check that kernel build files exist:

```bash

KVER=$(uname -r)

test -e "/lib/modules/$KVER/build" || echo "Install matching kernel headers"

```

If headers are missing, install the package matching uname -r, then reboot into that kernel.

## 2. Create the signing key

Run this once from the project root as your normal user:

```sh
make signing-key
```

Keep `build/signing/MOK.priv` private. The matching public certificate is `build/signing/MOK.der`.

## 3. Enroll the certificate

On a Secure Boot system using MOK, enroll the certificate once per machine. Choose a temporary password when prompted:

```sh
sudo mokutil --import build/signing/MOK.der
sudo reboot
```

In MOK Manager, select **Enroll MOK**, then **Continue**, then **Yes**. Enter the temporary password and reboot. Return to the project root and confirm enrollment:

```sh
mokutil --test-key build/signing/MOK.der
```

If your system accepts unsigned modules, you can skip steps 2 and 3. Systems using a different trust mechanism should follow their distribution's enrollment instructions.

## 4. Build and sign

```sh
make -j
```

Make builds all programs and signs the finished driver when both signing files are present. Reuse the same enrolled pair for later builds.

## 5. Load the driver

```sh
sudo make load
```

The load target skips an already loaded driver. To activate a rebuilt driver, run `sudo rmmod secret_driver` followed by `sudo make load`.

The program creates /dev/secret if missing. If its device numbers are wrong, recreate it as character device major 42, minor 0:

```bash

sudo rm /dev/secret

sudo mknod /dev/secret c 42 0

sudo chmod 666 /dev/secret

```

## 6. Unload module

```bash

sudo rmmod secret_driver

```

# AI

This project's documentation, code comments, structural organization, and setup guides were developed, refined, and structured in close collaboration with an AI assistant (Gemini).
