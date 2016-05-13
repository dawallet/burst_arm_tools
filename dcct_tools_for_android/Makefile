CFLAGS=-D_FILE_OFFSET_BITS=64

all:		plot32 plot plotavx2	

plot32:	plot.c shabal32.o helper32.o
		gcc -Wall -O2 -o plot32 plot.c shabal32.o helper32.o

plotavx2:	plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o 
		gcc -Wall -m64 -O2 -o plotavx2 plot.c shabal64.o helper64.o mshabal_sse4.o mshabal256_avx2.o -march=native -lpthread -std=gnu99 -DAVX2

helper32.o:	helper.c
		gcc -Wall -c -O2 -o helper32.o helper.c	

shabal32.o:	shabal32.s
		gcc -Wall -c -o shabal32.o shabal32.s

mshabal_sse4.o: mshabal_sse4.c
		gcc -Wall -m64 -c -O2 -march=native -o mshabal_sse4.o mshabal_sse4.c

mshabal256_avx2.o: mshabal256_avx2.c
		gcc -Wall -m64 -c -O2 -march=native -mavx2 -o mshabal256_avx2.o mshabal256_avx2.c

clean:
		rm -f mshabal_sse4.o mshabal256_avx2.o shabal64.o helper64.o plot plotavx2 
