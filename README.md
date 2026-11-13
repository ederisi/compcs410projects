# COMP.CS.410 projects
This repository houses a collection of low-level systems programming and C++ projects developed for COMP.CS.410, focusing on dynamic library loading, networking protocols, Linux kernel module development, and process orchestration.

## Project Structure
The project is organized into modular directories under a unified build system managed by a root Makefile:
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
├── compcs/                     # Process manager & service launcher
│   ├── processhelpers.h        # Fork, process group, signal handling, & process reaping
│   └── main.cpp                # Interactive terminal launcher CLI (run, status, kill)
│
├── Makefile                    # Centralized root build system
└── README.md                   # Project documentation and setup guide
```

## Component breakdown
### Compcs
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
The encryption executable is independent and is not registered in the launcher.

Run the existing launcher from its output directory so its relative executable
and shared-library paths resolve:

```sh
make run
```

## Cleanup

`make clean` removes `build/bin`, including all new build artifacts.




# Running the encryption driver on Linux

These steps target Linux systems with GNU make, a C/C++ toolchain, kernel build headers, and a kernel that supports external modules. Secure Boot signing steps use MOK, available on many shim-based distributions. Other systems may use a different key-enrollment process.

## 1. Install build requirements

Install your distribution's equivalents of:

- GNU make, GCC, and G++

- OpenSSL

- Kernel headers/development files matching the running kernel

- mokutil, if using MOK enrollment

Package examples:

~~~bash

# Fedora

sudo dnf install make gcc gcc-c++ openssl mokutil kernel-devel

# Debian or Ubuntu

sudo apt install build-essential libssl-dev mokutil linux-headers-$(uname -r)

# Arch Linux

sudo pacman -S base-devel openssl mokutil linux-headers

~~~

Check that kernel build files exist:

~~~bash

KVER=$(uname -r)

test -e "/lib/modules/$KVER/build" || echo "Install matching kernel headers"

~~~

If headers are missing, install the package matching uname -r, then reboot into that kernel.

## 2. Create a local signing key

Run from the project root:

~~~bash

openssl req -new -x509 -newkey rsa:2048 \

-keyout compcs410-mok.priv \

-outform DER -out compcs410-mok.der \

-nodes -days 3650 \

-subj "/CN=COMP.CS.410 local module signing/"

chmod 600 compcs410-mok.priv

~~~

Never commit or share compcs410-mok.priv. Each user should create and enroll their own key. The certificate may be shared, but its private key must stay private.

## 3. Enroll the certificate when Secure Boot enforces module signatures

Check Secure Boot state using your distribution's tool. On shim-based systems with mokutil:

~~~bash

mokutil --sb-state

sudo mokutil --import "$PWD/compcs410-mok.der"

sudo systemctl reboot

~~~

At reboot, MOK Manager asks to enroll the certificate. Choose **Enroll MOK**, confirm, enter the password set during import, then reboot. Verify:

~~~bash

sudo mokutil --test-key "$PWD/compcs410-mok.der"

~~~

Skip this step if module signature enforcement is off. If your distribution does not use MOK, follow its documented key-enrollment process.

## 4. Build executable and module

From project root:

~~~bash

make encryption

make module KDIR="/lib/modules/$(uname -r)/build"

~~~

Executable: build/bin/encryption

Module: build/bin/82-encryption/secret\\\_driver.ko

## 5. Sign and load module

If Secure Boot enforcement requires a signature, sign the final module after every rebuild. The kernel build tree usually provides scripts/sign-file:

~~~bash

KVER=$(uname -r)

SIGN="/lib/modules/$KVER/build/scripts/sign-file"

MODULE="$PWD/build/bin/82-encryption/secret_driver.ko"

"SIGN" sha256 "PWD/compcs410-mok.priv" "PWD/compcs410-mok.der" "MODULE"

~~~

Load the module:

~~~bash

sudo insmod "$MODULE"

grep my_device_driver /proc/devices

~~~

If sign-file is elsewhere, use the copy provided by your distribution's matching kernel development package. If the module is already loaded, unload it before loading a rebuilt copy:

~~~bash

sudo rmmod secret_driver

sudo insmod "$MODULE"

~~~

The program creates /dev/secret if missing. If its device numbers are wrong, recreate it as character device major 42, minor 0:

~~~bash

sudo rm /dev/secret

sudo mknod /dev/secret c 42 0

sudo chmod 666 /dev/secret

~~~

## 6. Set key and encrypt

~~~bash

sudo build/bin/encryption --setkey 42

sudo build/bin/encryption --encrypt "first message" "second message"

~~~

The key stays in driver memory. It resets when the module unloads or the system reboots. Set it again after each reload.

## 7. Unload module

~~~bash

sudo rmmod secret_driver

~~~


# AI
This project's documentation, code comments, structural organization, and setup guides were developed, refined, and structured in close collaboration with an AI assistant (Gemini).
