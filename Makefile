all: ipk-mtrip
ipk-mtrip: ipk-mtrip.o

	gcc -std=c99 -o ipk-mtrip ipk-mtrip.c -lm

clean: 
	rm -f ipk-mtrip ipk-mtrip.o