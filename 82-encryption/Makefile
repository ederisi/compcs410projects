KDIR ?= /lib/modules/$(shell uname -r)/build
CXX  ?= g++
CC   ?= gcc

CXXFLAGS ?= -Wall -Wextra -std=c++20
obj-m := sd.o

.PHONY: all module clean

all: main module

main: main.cpp sd.h
	$(CXX) $(CXXFLAGS) main.cpp -o main

module: sd.c sd.h
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	$(RM) main