CFLAGS=-D_FILE_OFFSET_BITS=64

all:		plot32

plot32:	plot.c shabal32.o helper32.o
		gcc -Wall -O2 -o plot32 plot.c shabal32.o helper32.o

helper32.o:	helper.c
		gcc -Wall -c -O2 -o helper32.o helper.c	

shabal32.o:	shabal32.s
		gcc -Wall -c -o shabal32.o shabal32.s
		
clean:
		rm -f mshabal_sse4.o mshabal256_avx2.o shabal64.o helper64.o plot plotavx2 
