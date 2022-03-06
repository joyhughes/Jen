lux: lux.o func_node.o scene.o fimage.o vfield.o joy_io.o frgb.o vect2.o
	gcc -o lux lux.o func_node.o scene.o fimage.o vfield.o joy_io.o frgb.o vect2.o

lux.o: lux.c fimage.h vfield.h func_node.h frgb.h vect2.h
	gcc -o lux.o -c lux.c

hyper.o: hyper.c fimage.h vfield.h func_node.h frgb.h vect2.h
	gcc -o hyper.o -c hyper.c

melt.o: melt.c fimage.h vfield.h func_node.h frgb.h vect2.h
	gcc -o melt.o -c melt.c

warp.o: warp.c fimage.h func_node.h frgb.h vect2.h
	gcc -o warp.o -c warp.c

scene.o: scene.c fimage.h func_node.h frgb.h vect2.h joy_io.h
	gcc -o scene.o -c scene.c

fimage.o: fimage.c fimage.h func_node.h frgb.h vect2.h
	gcc -o fimage.o  -c fimage.c

vfield.o: vfield.c vfield.h func_node.h frgb.h vect2.h
	gcc -o vfield.o  -c vfield.c

func_node.o: func_node.c func_node.h joy_io.h frgb.h vect2.h
	gcc -o func_node.o -c func_node.c

joy_io.o: joy_io.c joy_io.h frgb.h vect2.h
	gcc -o joy_io.o -c joy_io.c

frgb.o: frgb.c frgb.h
	gcc -o frgb.o -c frgb.c 

vect2.o: vect2.c vect2.h
	gcc -o vect2.o -c vect2.c