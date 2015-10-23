# Sample Makefile for a GKrellM plugin

GTK_INCLUDE = `pkg-config gtk+-2.0 --cflags`
GTK_LIB = `pkg-config gtk+-2.0 --libs`

FLAGS = -O2 -Wall -fPIC $(GTK_INCLUDE)
LIBS = $(GTK_LIB)

LFLAGS = -shared

CC = gcc $(CFLAGS) $(FLAGS)

OBJS = demo4.o

demo4.so: $(OBJS)
	$(CC) $(OBJS) -o demo4.so $(LFLAGS) $(LIBS)

clean:
	rm -f *.o core *.so* *.bak *~

#IMAGES = demo4_slider.xpm

demo4.o: demo4.c

