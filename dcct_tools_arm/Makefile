CFLAGS=-D_FILE_OFFSET_BITS=32


linux32:	plot32 optimize32 mine32 mine_pool_all32 mine_pool_share32

all:		 linux32

dist:		 linux32 
		rm -f bin/* shabal64.o shabal32.o dcct_miner.tgz
		mv plot optimize mine mine_pool_all plot32 optimize32 mine32 mine_pool_all32 mine_pool_share mine_pool_share32 optimize.exe bin
		cp mine_uray.sh mine_dev_v1.sh mine_dev_v2.sh bin
		tar -czf dcct_miner.tgz *

plot32:		plot.c shabal32.o helper32.o
		gcc -Wall -O2 -o plot32 plot.c shabal32.o helper32.o -pthread

mine32:		mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DSOLO -o mine32 mine.c shabal32.o helper32.o -pthread 

mine_pool_all32:	mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DURAY_POOL -o mine_pool_all32 mine.c shabal32.o helper32.o -pthread 

mine_pool_share32:	mine.c shabal32.o helper32.o
		gcc -Wall -O2 -DSHARE_POOL -o mine_pool_share32 mine.c shabal32.o helper32.o -pthread 

optimize32:	optimize.c helper32.o
		gcc -Wall -O2 -o optimize32 optimize.c helper32.o	

helper32.o:	helper.c
		gcc -Wall -c -O2 -o helper32.o helper.c		

shabal32.o:	shabal32.s
		gcc -Wall -c -o shabal32.o shabal32.s

clean:
		rm -f shabal64.o shabal32.o helper64.o helper32.o plot plot32 optimize optimize32 mine mine32 mine_pool_all mine_pool_all32 mine_pool_share mine_pool_share32 helper32.o helper64.o optimize.exe dcct_miner.tgz
