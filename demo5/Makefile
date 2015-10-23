# Sample Makefile for a GKrellM combo client/server plugin.
# Run "make help" to see all the make options.
#

INSTALLROOT ?= $(DESTDIR)$(PREFIX)

ifeq ($(INSTALLROOT),)
	INSTALLROOT = /usr/local
endif

INSTALLDIR ?= $(INSTALLROOT)/lib/gkrellm2/plugins
SERVER_INSTALLDIR ?= $(INSTALLROOT)/lib/gkrellm2/plugins-gkrellmd
INSTALL ?= install
STRIP ?= -s


# Stuff for the main gkrellm plugin compile.
#
GTK_INCLUDE = `pkg-config gtk+-2.0 --cflags`
GTK_LIBS = `pkg-config gtk+-2.0 --libs`


# Stuff for the gkrellmd server plugin compile.  Provide for an optional
# linking to glib 1.2 since on some servers gkrellmd is built for glib 1.2.
#
ifeq ($(glib12),yes)
WANT_GLIB12 = yes
endif
ifeq ($(glib12),1)
WANT_GLIB12 = yes
endif

ifeq ($(WANT_GLIB12),yes)
GLIB_INCLUDE = `glib-config --cflags`
GLIB_LIBS = `glib-config --libs`
CONFIGURE_ARGS += --with-glib12
else
GLIB_INCLUDE = `pkg-config --cflags glib-2.0`
GLIB_LIBS = `pkg-config --libs glib-2.0`
endif


# Run the configure script to determine if the gkrellmd.h header file
# is installed.
#
DUMMY_VAR := $(shell ./configure $(CONFIGURE_ARGS))
HAVE_GKRELLMD = $(shell grep -c HAVE_GKRELLMD configure.h)

ifneq ($(HAVE_GKRELLMD),1)
WARN = $(warning Note: cannot compile server plugin.  See configure.log.)
endif



LINK_FLAGS = -shared
CC = gcc $(CFLAGS) -O2 -Wall -fPIC

OBJS = demo5.o data.o
SERVER_OBJS = demo5d.o data.o

PLUGIN = demo5.so

ifeq ($(HAVE_GKRELLMD),1)
	SERVER_PLUGIN = demo5d.so
endif


all: $(PLUGIN) $(SERVER_PLUGIN)

warn: ; $(WARN)

$(PLUGIN): $(OBJS) warn
	$(CC) $(OBJS) -o $(PLUGIN) $(LINK_FLAGS) $(GTK_LIBS)


# Target for making the server plugin only
server: $(SERVER_PLUGIN) warn
	@echo ""

$(SERVER_PLUGIN): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER_PLUGIN) $(LINK_FLAGS) $(GLIB_LIBS)


# Target to install both the gkrellm plugin and the gkrellmd server plugin.
#
install:
	$(INSTALL) -d -m 755 $(INSTALLDIR)
	$(INSTALL) -c $(STRIP) -m 755 $(PLUGIN) $(INSTALLDIR)
	$(INSTALL) -d -m 755 $(SERVER_INSTALLDIR)
	$(INSTALL) -c $(STRIP) -m 755 $(SERVER_PLUGIN) $(SERVER_INSTALLDIR)

uninstall:
	rm -f $(INSTALLDIR)/$(PLUGIN)
	rm -f $(SERVER_INSTALLDIR)/$(SERVER_PLUGIN)

# Target for installing the server plugin only.
#
install-server:
	$(INSTALL) -d -m 755 $(SERVER_INSTALLDIR)
	$(INSTALL) -c $(STRIP) -m 755 $(SERVER_PLUGIN) $(SERVER_INSTALLDIR)

uninstall-server:
	rm -f $(SERVER_INSTALLDIR)/$(SERVER_PLUGIN)



help:
	@echo ""
	@echo "make [glib12=yes]"
	@echo "    Make the gkrellm demo5.so plugin and if the gkrellmd server"
	@echo "    development header file is installed, also make a demo5d.so"
	@echo "    server plugin.  The server plugin may be compiled to use"
	@echo "    glib 1.2 if the glib12=yes arg is given."
	@echo ""
	@echo "make server [glib12=yes]"
	@echo "    Make the demo5d.so server plugin only.  Requires the gkrellmd.h"
	@echo "    development header file from gkrellm >= 2.2.0 to be installed."
	@echo ""
	@echo "make install [DESTDIR=dir] [PREFIX=dir]"
	@echo "    Install both demo5.so and demo5d.so into the default"
	@echo "    /usr/local/lib/gkrellm2/plugins for demo5.so and"
	@echo "    /usr/local/lib/gkrellm2/plugins-gkrellmd for demo5d.so."
	@echo "    Use DESTDIR and/or PREFIX to  install the plugins under"
	@echo "    DESTDIR/PREFIX/lib/gkrellm2."
	@echo ""
	@echo "make install-server [DESTDIR=dir] [PREFIX=dir]"
	@echo "    Install only the demo5d.so server plugin."
	@echo ""
	@echo "make uninstall"
	@echo "make uninstall-server"
	@echo ""


clean:
	rm -f *.o core *.so* *.bak *~ configure.h configure.log

demo5.o:	demo5.c
	$(CC) -c demo5.c -o demo5.o $(GTK_INCLUDE)

demo5d.o:	demo5d.c
	$(CC) -c demo5d.c -o demo5d.o $(GLIB_INCLUDE)

data.o:		data.c
	$(CC) -c data.c -o data.o $(GLIB_INCLUDE)

data.o:		data.c demo5-common.h
demo5.o:	demo5.c demo5-common.h
demo5d.o:	demo5d.c demo5-common.h
