CFLAGS=-g
LDLIBS=-lm
CC=gcc

lux: lux.o func_node.o scene.o fimage.o vfield.o joy_io.o frgb.o vect2.o

warp: warp.o func_node.o fimage.o vfield.o joy_io.o frgb.o vect2.o

melt: melt.o func_node.o fimage.o vfield.o joy_io.o frgb.o vect2.o

hyper: hyper.o func_node.o fimage.o vfield.o joy_io.o frgb.o vect2.o
	
life: life.o frgb.o 

life.o: life.c fimage.h

lux.o: lux.c scene.h fimage.h vfield.h func_node.h frgb.h vect2.h

hyper.o: hyper.c fimage.h vfield.h func_node.h frgb.h vect2.h

melt.o: melt.c fimage.h vfield.h func_node.h frgb.h vect2.h

warp.o: warp.c fimage.h func_node.h frgb.h vect2.h

scene.o: scene.c fimage.h func_node.h frgb.h vect2.h joy_io.h

fimage.o: fimage.c fimage.h func_node.h frgb.h vect2.h

vfield.o: vfield.c vfield.h func_node.h frgb.h vect2.h

func_node.o: func_node.c func_node.h joy_io.h frgb.h vect2.h

joy_io.o: joy_io.c joy_io.h frgb.h vect2.h

frgb.o: frgb.c frgb.h

vect2.o: vect2.c vect2.h

clean:
	touch fake.o && rm -rf *.o lux warp melt hyper life
