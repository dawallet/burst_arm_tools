CFLAGS=-D_FILE_OFFSET_BITS=64


linux32:	plot32 optimize32 mine32 mine_pool_all32 mine_pool_share32

all:	    linux32

dist:		linux32
		rm -f bin/* shabal64.o shabal32.o dcct_miner.tgz
		mv plot32 optimize32 mine32 mine_pool_all32 mine_pool_share32 bin
		cp mine_uray.sh mine_dev_v1.sh mine_dev_v2.sh bin
		tar -czf dcct_miner.tgz *

plot32:		plot.c shabal32.o helper32.o
		gcc -Wall -O2 -o plot32 plot.c shabal32.o helper32.o

mine32:		mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DSOLO -o mine32 mine.c shabal32.o helper32.o 

mine_pool_all32:	mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DURAY_POOL -o mine_pool_all32 mine.c shabal32.o helper32.o

mine_pool_share32:	mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DSHARE_POOL -o mine_pool_share32 mine.c shabal32.o helper32.o

optimize32:	optimize.c helper32.o
		gcc -Wall -O2 -o optimize32 optimize.c helper32.o

helper32.o:	helper.c
		gcc -Wall -c -O2 -o helper32.o helper.c		

shabal64.o:	shabal64.s
		gcc -Wall -m64 -c -o shabal64.o shabal64.s

shabal32.o:	shabal32.s
		gcc -Wall -c -o shabal32.o shabal32.s

clean:
		rm -f shabal32.o helper32.o plot32 optimize32 mine32 mine_pool_all32 mine_pool_share32 helper32.o dcct_miner.tgz
