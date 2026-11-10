.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

CXX ?= g++
CXXFLAGS ?= -Wall -Wextra
KDIR ?= /lib/modules/$(shell uname -r)/build

BIN := build/bin
OBJ := $(BIN)/obj
MODULE_DIR := $(BIN)/82-encryption

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

# Stage module sources so kbuild also keeps every generated file under build/bin.
$(MODULE_DIR)/sd.c: 82-encryption/sd.c
	mkdir -p "$(@D)"
	cp $< $@

$(MODULE_DIR)/sd.h: 82-encryption/sd.h
	mkdir -p "$(@D)"
	cp $< $@

$(MODULE_DIR)/crypto.c: 82-encryption/crypto.c
	mkdir -p "$(@D)"
	cp $< $@

$(MODULE_DIR)/Kbuild: Makefile
	mkdir -p "$(@D)"
	printf '%s\n' 'obj-m := secret_driver.o' 'secret_driver-y := sd.o crypto.o' > $@

module: $(MODULE_DIR)/sd.c $(MODULE_DIR)/sd.h $(MODULE_DIR)/crypto.c $(MODULE_DIR)/Kbuild
	$(MAKE) -C "$(KDIR)" M="$(abspath $(MODULE_DIR))" modules

# Existing executable and shared-library paths are relative to the working directory.
run: launcher dlopen wsserver
	cd "$(BIN)" && ./launcher

clean:
	rm -rf -- build/bin

-include $(OBJECTS:.o=.d)
