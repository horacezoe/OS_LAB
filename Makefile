all:
	nasm -f elf my_print.asm -o my_print.o
	gcc -m32 main.c my_print.o -o Lab2 -g
	rm my_print.o
	./Lab2