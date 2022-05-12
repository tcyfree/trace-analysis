# ssdsim linux support
all:ssd 
	
clean:
	rm -f ssd *.o *.out *.dat *~
.PHONY: clean

ssd: ssd.o avlTree.o flash.o initialize.o pagemap.o insert_to_buffer.o
	cc -g -o ssd ssd.o avlTree.o flash.o initialize.o pagemap.o insert_to_buffer.o
ssd.o: flash.h initialize.h pagemap.h
	gcc -c -g ssd.c
flash.o: pagemap.h
	gcc -c -g flash.c
initialize.o: avlTree.h pagemap.h
	gcc -c -g initialize.c
pagemap.o: initialize.h
	gcc -c -g pagemap.c
avlTree.o: 
	gcc -c -g avlTree.c
insert_to_buffer.o: insert_to_buffer.h
		gcc -c -g insert_to_buffer.c
