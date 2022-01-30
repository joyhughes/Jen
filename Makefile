lux2: lux2.o func_node.o vect2.o
	gcc -o lux2 lux2.o func_node.o vect2.o

lux2.o: lux2.c
	gcc -o lux2.o -c lux2.c

func_node.o: func_node.c
	gcc -o func_node.o -c func_node.c

fimage.o

vect2.o: vect2.c
	gcc -o vect2.o -c vect2.c