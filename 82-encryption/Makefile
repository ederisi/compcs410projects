# Set 'all' as the default goal when running 'make' without arguments
.DEFAULT_GOAL := all
# Declare phony targets to ensure rules execute even if same files exist
.PHONY: all main encryption module clean

# The default target builds both the encryption test program and the kernel module
all: encryption module

# Map the 'main' target alias
main: encryption

# Delegate build and clean commands to parent Makefile
encryption module clean:
	$(MAKE) -C .. $@
