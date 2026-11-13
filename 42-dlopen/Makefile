# Set 'all' as the default goal when running 'make' without args
.DEFAULT_GOAL := all

# Declare phony targets to ensure rules execute 
# (even with same name file)
.PHONY: all dlopen mathlib mathlib2 clean

# The default target builds
all: dlopen

# Delegate build and clean commands for dlopen,...
# to the Makefile located in the parent dir
dlopen mathlib mathlib2 clean:
	$(MAKE) -C .. $@
