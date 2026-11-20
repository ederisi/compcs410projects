.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

CXX ?= g++
CXXFLAGS ?= -Wall -Wextra
KDIR ?= /lib/modules/$(shell uname -r)/build

BIN := build/bin
OBJ := $(BIN)/obj
MODULE_DIR := $(BIN)/82-encryption
KBUILD_DIR := $(MODULE_DIR)/kbuild
DRIVER := $(MODULE_DIR)/secret_driver.ko
RAW_DRIVER := $(KBUILD_DIR)/secret_driver.ko
SIGNED_DRIVER := $(MODULE_DIR)/signed/secret_driver.ko
SIGN_KEY := build/signing/MOK.priv
SIGN_CERT := build/signing/MOK.der
SIGN_FILE ?= $(KDIR)/scripts/sign-file

LAUNCHER_OBJECTS := $(OBJ)/main.o
DLOPEN_OBJECTS := $(OBJ)/42-dlopen/main.o $(OBJ)/42-dlopen/dlopen.o
WSSERVER_OBJECTS := $(OBJ)/74-wsserver/main.o $(OBJ)/74-wsserver/websocket.o
ENCRYPTION_OBJECTS := $(OBJ)/82-encryption/main.o
LIBRARY_OBJECTS := $(OBJ)/42-dlopen/mathlib.o $(OBJ)/42-dlopen/mathlib2.o
OBJECTS := $(LAUNCHER_OBJECTS) $(DLOPEN_OBJECTS) $(WSSERVER_OBJECTS) $(ENCRYPTION_OBJECTS) $(LIBRARY_OBJECTS)

.PHONY: all launcher dlopen wsserver mathlib mathlib2 encryption module clean run

# Build userspace programs and the encryption kernel module.
all: launcher dlopen wsserver encryption module

launcher: $(BIN)/launcher
dlopen: $(BIN)/dlopen $(BIN)/libmathlib.so $(BIN)/libmathlib2.so
wsserver: $(BIN)/wsserver
mathlib: $(BIN)/libmathlib.so
mathlib2: $(BIN)/libmathlib2.so
encryption: $(BIN)/encryption

$(BIN)/launcher: $(LAUNCHER_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN)/dlopen: $(DLOPEN_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -ldl -o $@

$(BIN)/wsserver: $(WSSERVER_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN)/encryption: $(ENCRYPTION_OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile shared libraries with position-independent code.
$(LIBRARY_OBJECTS): PRIVATE_FLAGS := -fPIC

$(BIN)/libmathlib.so: $(OBJ)/42-dlopen/mathlib.o
	$(CXX) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@

$(BIN)/libmathlib2.so: $(OBJ)/42-dlopen/mathlib2.o
	$(CXX) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@

$(OBJ)/main.o: PRIVATE_DEFINES := -DDLOPEN_NAME=\"dlopen\" -DWSSERVER_NAME=\"wsserver\"
$(OBJ)/42-dlopen/main.o $(OBJ)/42-dlopen/dlopen.o: PRIVATE_DEFINES := -DMATHLIB_SHARED_NAME=\"libmathlib.so\"

$(OBJ)/%.o: %.cpp Makefile
	mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(PRIVATE_DEFINES) $(CXXFLAGS) -std=c++20 $(PRIVATE_FLAGS) -MMD -MP -c $< -o $@

# Keep kbuild's original module as the input for every signature.
$(KBUILD_DIR)/%.c: 82-encryption/%.c
	mkdir -p "$(@D)"
	cp $< $@

$(KBUILD_DIR)/sd.h: 82-encryption/sd.h
	mkdir -p "$(@D)"
	cp $< $@

$(KBUILD_DIR)/Kbuild: Makefile
	mkdir -p "$(@D)"
	printf '%s\n' 'obj-m := secret_driver.o' 'secret_driver-y := sd.o crypto.o' > $@

ifeq ($(words $(wildcard $(SIGN_KEY) $(SIGN_CERT))),2)
MODULE_SOURCE := $(SIGNED_DRIVER)
else
MODULE_SOURCE := $(RAW_DRIVER)
endif

module: $(KBUILD_DIR)/sd.c $(KBUILD_DIR)/sd.h $(KBUILD_DIR)/crypto.c $(KBUILD_DIR)/Kbuild
	$(MAKE) -C "$(KDIR)" M="$(abspath $(KBUILD_DIR))" modules
ifeq ($(MODULE_SOURCE),$(SIGNED_DRIVER))
	$(MAKE) "$(SIGNED_DRIVER)"
else
	@echo "Skipping local signing: requires $(SIGN_KEY) and $(SIGN_CERT)."
endif
	@cmp -s "$(MODULE_SOURCE)" "$(DRIVER)" || { cp "$(MODULE_SOURCE)" "$(DRIVER).tmp" && mv "$(DRIVER).tmp" "$(DRIVER)"; }

# Generate the local module signing pair once, before running make -j.
.PHONY: signing-key
signing-key:
	@set -eu; umask 077; \
	if [ -s "$(SIGN_KEY)" ] && [ -s "$(SIGN_CERT)" ]; then \
		echo "Reusing $(SIGN_KEY) and $(SIGN_CERT)."; \
	elif [ -e "$(SIGN_KEY)" ] || [ -e "$(SIGN_CERT)" ]; then \
		echo "Incomplete signing pair. Restore the matching file or move existing files aside before retrying." >&2; exit 1; \
	else \
		mkdir -p "$(dir $(SIGN_KEY))"; \
		tmp=$$(mktemp -d "$(dir $(SIGN_KEY)).mok.XXXXXX"); \
		trap 'rm -f "$$tmp/key" "$$tmp/cert"; rmdir "$$tmp"' EXIT; \
		openssl req -new -x509 -newkey rsa:2048 -sha256 -nodes -days 3650 \
			-subj "/CN=COMP.CS.410 local module signing/" \
			-keyout "$$tmp/key" -outform DER -out "$$tmp/cert"; \
		ln -T "$$tmp/key" "$(SIGN_KEY)"; \
		ln -T "$$tmp/cert" "$(SIGN_CERT)"; \
	fi
	@echo "For MOK enrollment, run sudo mokutil --import $(SIGN_CERT), then reboot and select Enroll MOK."

$(SIGNED_DRIVER): $(RAW_DRIVER) $(SIGN_KEY) $(SIGN_CERT)
	mkdir -p "$(@D)"
	"$(SIGN_FILE)" sha256 "$(SIGN_KEY)" "$(SIGN_CERT)" "$<" "$@.tmp"
	mv "$@.tmp" "$@"

.PHONY: load run-dlopen run-wsserver run-encryption
load:
	@if [ -d /sys/module/secret_driver ]; then \
		echo "secret_driver is already loaded."; \
	elif [ -f "$(DRIVER)" ]; then \
		insmod "$(DRIVER)"; \
	else \
		echo "Missing $(DRIVER). Run make -j first." >&2; exit 1; \
	fi

# Run each program from its working directory. Executables stay in BIN.
run: launcher dlopen wsserver encryption
	cd "$(BIN)" && ./launcher $(ARGS)

run-dlopen: dlopen
	cd 42-dlopen && "$(abspath $(BIN)/dlopen)" $(ARGS)

run-wsserver: wsserver
	cd 74-wsserver && "$(abspath $(BIN)/wsserver)" $(ARGS)

run-encryption: encryption
	cd "$(BIN)" && ./encryption $(ARGS)

clean:
	rm -rf -- build/bin

-include $(OBJECTS:.o=.d)
