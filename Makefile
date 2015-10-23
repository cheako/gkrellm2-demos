#!/usr/bin/env make

all: demo1.so demo1-mod.so demo2.so demo3.so demo-alert.so

GTK_CFLAGS:=$(shell pkg-config --cflags gtk+-2.0)

%.o: %.c
	gcc -Wall -fPIC $(GTK_CFLAGS) -c $?

%.so: %.o
	gcc -shared -o $@ $?

%.test: %.so
	gkrellm -p $<

.PHONY: clean
clean:
	rm *o
