all: libsimplefs.a app

simplefs.o: simplefs.c
	gcc -Wall -c simplefs.c -lm
libsimplefs.a: 	simplefs.o
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

app:   app.c
	gcc -Wall -o app app.c  -L. -lsimplefs -lm

clean: 
	rm -fr *.o *.a *~ a.out app  vdisk1.bin
