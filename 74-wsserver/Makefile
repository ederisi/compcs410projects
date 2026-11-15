# Set 'all' as default target when running 'make' without args
.DEFAULT_GOAL := all

# Declare phony targets to ensure rules even with multiple files
.PHONY: all wsserver clean

# Default target builds
all: wsserver

# Delegate build and clean commands to the Makefile in parent dir
wsserver clean:
	$(MAKE) -C .. $@

.PHONY: run
run:
	$(MAKE) -C .. run-wsserver
