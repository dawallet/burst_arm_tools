/*
	Burstcoin plot generator V2
	Creates version 2 plotfiles
	Author: Markus Tervooren <info@bchain.info>
	Burst: BURST-R5LP-KEL9-UYLG-GFG6T

	Modified version originally written by Cerr Janror <cerr.janror@gmail.com> : https://github.com/BurstTools/BurstSoftware.git

	Author: Mirkic7 <mirkic7@hotmail.com>
	Burst: BURST-RQW7-3HNW-627D-3GAEV

	Original author: Markus Tervooren <info@bchain.info>
	Burst: BURST-R5LP-KEL9-UYLG-GFG6T

	Implementation of Shabal is taken from:
	http://www.shabal.com/?p=198

	Usage: ./plot <public key> <start nonce> <nonces> <stagger size> <threads>
*/

#define USE_MULTI_SHABAL

#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "shabal.h"

#include "mshabal.h"
#include "helper.h"

#define DEFAULTDIR	"plots/"

// Not to be changed below this
#define SCOOPS		65536
#define SCOOPSIZE	16
#define PLOT_SIZE	(SCOOPSIZE * SCOOPS)
#define HASH_SIZE	32
#define HASH_CAP	2048

unsigned long long addr = 0;
unsigned long long startnonce = 0;
unsigned int nonces = 0;
unsigned int staggersize = 0;
unsigned int threads = 0;
unsigned int noncesperthread;
unsigned int selecttype = 0;
unsigned int asyncmode = 0;
unsigned long long starttime;
int ofd, run, lastrun;

char *cache, *wcache, *acache[2];
char *outputdir = DEFAULTDIR;

#define SET_NONCE(gendata, nonce) \
  xv = (char*)&nonce; \
  gendata[PLOT_SIZE + 8] = xv[7]; gendata[PLOT_SIZE + 9] = xv[6]; gendata[PLOT_SIZE + 10] = xv[5]; gendata[PLOT_SIZE + 11] = xv[4]; \
  gendata[PLOT_SIZE + 12] = xv[3]; gendata[PLOT_SIZE + 13] = xv[2]; gendata[PLOT_SIZE + 14] = xv[1]; gendata[PLOT_SIZE + 15] = xv[0]


int mnonce(unsigned long long int addr, unsigned long long int nonce) {
    char final1[32], final2[32], final3[32], final4[32];
//    char gendata1[16 + PLOT_SIZE], gendata2[16 + PLOT_SIZE], gendata3[16 + PLOT_SIZE], gendata4[16 + PLOT_SIZE];
    char finalPosition1[32], finalPosition2[32], finalPosition3[32], finalPosition4[32];
    char *gendata1 = (char*)malloc(16 + PLOT_SIZE); char *gendata2 = (char*)malloc(16 + PLOT_SIZE); char *gendata3 = (char*)malloc(16 + PLOT_SIZE); char *gendata4 = (char*)malloc(16 + PLOT_SIZE);

    char *xv = (char*)&addr;

    gendata1[PLOT_SIZE] = xv[7]; gendata1[PLOT_SIZE + 1] = xv[6]; gendata1[PLOT_SIZE + 2] = xv[5]; gendata1[PLOT_SIZE + 3] = xv[4];
    gendata1[PLOT_SIZE + 4] = xv[3]; gendata1[PLOT_SIZE + 5] = xv[2]; gendata1[PLOT_SIZE + 6] = xv[1]; gendata1[PLOT_SIZE + 7] = xv[0];

    for (int i = PLOT_SIZE; i <= PLOT_SIZE + 7; ++i)
    {
      gendata2[i] = gendata1[i];
      gendata3[i] = gendata1[i];
      gendata4[i] = gendata1[i];
    }

    unsigned long long int nonce2 = nonce + 1;
    unsigned long long int nonce3 = nonce + 2;
    unsigned long long int nonce4 = nonce + 3;

    SET_NONCE(gendata1, nonce);
    SET_NONCE(gendata2, nonce2);
    SET_NONCE(gendata3, nonce3);
    SET_NONCE(gendata4, nonce4);

    mshabal_context x;
    int i, len;



    // XOR with final
    unsigned long long *start1 = (unsigned long long*)gendata1;
    unsigned long long *start2 = (unsigned long long*)gendata2;
    unsigned long long *start3 = (unsigned long long*)gendata3;
    unsigned long long *start4 = (unsigned long long*)gendata4;
    unsigned long long *fint1 = (unsigned long long*)&final1;
    unsigned long long *fint2 = (unsigned long long*)&final2;
    unsigned long long *fint3 = (unsigned long long*)&final3;
    unsigned long long *fint4 = (unsigned long long*)&final4;


    // Start XOR from scoop 0 on, in 32 Byte steps:
    for(i = 0; i < PLOT_SIZE; i += 32) {
         *start1 ^= fint1[0]; *start2 ^= fint2[0]; *start3 ^= fint3[0]; *start4 ^= fint4[0]; 
         start1++; start2++; start3++; start4++;
         *start1 ^= fint1[1]; *start2 ^= fint2[1]; *start3 ^= fint3[1]; *start4 ^= fint4[1]; 
         start1++; start2++; start3++; start4++;
         *start1 ^= fint1[2]; *start2 ^= fint2[2]; *start3 ^= fint3[2]; *start4 ^= fint4[2]; 
         start1++; start2++; start3++; start4++;
         *start1 ^= fint1[3]; *start2 ^= fint2[3]; *start3 ^= fint3[3]; *start4 ^= fint4[3]; 
         start1++; start2++; start3++; start4++;

	 // Get position of scoops to mix in:
	 int position1, position2, position3, position4;
	

    // Get offset inside block:
	unsigned long long noncePosition1 = nonce       % staggersize;
	unsigned long long noncePosition2 = (nonce + 1) % staggersize;
	unsigned long long noncePosition3 = (nonce + 2) % staggersize;
	unsigned long long noncePosition4 = (nonce + 3) % staggersize;

	// Sort them using new distribution scheme:
        for(i = 0; i < SCOOPS; i++) {
                memcpy( &cache[((i + SCOOPS - noncePosition1) % SCOOPS) * staggersize * 16 + (noncePosition1) * 16], &gendata1[i * 16], 16);
                memcpy( &cache[((i + SCOOPS - noncePosition2) % SCOOPS) * staggersize * 16 + (noncePosition2) * 16], &gendata2[i * 16], 16);
                memcpy( &cache[((i + SCOOPS - noncePosition3) % SCOOPS) * staggersize * 16 + (noncePosition3) * 16], &gendata3[i * 16], 16);
                memcpy( &cache[((i + SCOOPS - noncePosition4) % SCOOPS) * staggersize * 16 + (noncePosition4) * 16], &gendata4[i * 16], 16);
	}
	free(gendata4); free(gendata3); free(gendata2); free(gendata1);

        return 0;
}

void nonce(unsigned long long int addr, unsigned long long int nonce) {
	char final[32];
	char finalPosition[32];
	char gendata[16 + PLOT_SIZE];

	char *xv = (char*)&addr;
	
	gendata[PLOT_SIZE] = xv[7]; gendata[PLOT_SIZE+1] = xv[6]; gendata[PLOT_SIZE+2] = xv[5]; gendata[PLOT_SIZE+3] = xv[4];
	gendata[PLOT_SIZE+4] = xv[3]; gendata[PLOT_SIZE+5] = xv[2]; gendata[PLOT_SIZE+6] = xv[1]; gendata[PLOT_SIZE+7] = xv[0];

	xv = (char*)&nonce;

	gendata[PLOT_SIZE+8] = xv[7]; gendata[PLOT_SIZE+9] = xv[6]; gendata[PLOT_SIZE+10] = xv[5]; gendata[PLOT_SIZE+11] = xv[4];
	gendata[PLOT_SIZE+12] = xv[3]; gendata[PLOT_SIZE+13] = xv[2]; gendata[PLOT_SIZE+14] = xv[1]; gendata[PLOT_SIZE+15] = xv[0];

	shabal_context x;
	int i, len;

	for(i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
		shabal_init(&x, 256);
		len = PLOT_SIZE + 16 - i;
		if(len > HASH_CAP)
			len = HASH_CAP;
		shabal(&x, &gendata[i], len);
		shabal_close(&x, 0, 0, &gendata[i - HASH_SIZE]);
	}
	
	// Get final hash
	shabal_init(&x, 256);
	shabal(&x, gendata, 16 + PLOT_SIZE);
	shabal_close(&x, 0, 0, final);

	// XOR with final
	unsigned long long *start = (unsigned long long*)gendata;
	unsigned long long *fint = (unsigned long long*)&final;

	// Start XOR from scoop 0 on, in 32 Byte steps:
	for(i = 0; i < PLOT_SIZE; i += 32) {
		*start ^= fint[0]; start ++;
		*start ^= fint[1]; start ++;
		*start ^= fint[2]; start ++;
		*start ^= fint[3]; start ++;
		
		// Get position of scoops to mix in:
		int position;
		if(i == 0) {
			position = 0;
		} else {
			shabal_init(&x, 256);
			shabal(&x, final, 32);
			shabal(&x, &gendata[i], 32);
			shabal_close(&x, 0, 0, finalPosition);
		
			position = ((unsigned char)finalPosition[0] + 256 * (unsigned char)finalPosition[1] + 256 * 256 * (unsigned char)finalPosition[2]) % (i / 32); 
		}

		// Create new final hash:
		shabal_init(&x, 256);
		shabal(&x, final, 32);
		shabal(&x, &gendata[position * 32], 32);
		shabal_close(&x, 0, 0, final);
	}

	// Fake plot for verification:
/*	unsigned long long *pointer = (unsigned long long*)gendata;
	for(i = 0; i < SCOOPS; i++) {
		*pointer = nonce; pointer++;
		*pointer = i; pointer++;
	}*/

	// Get offset inside block:
	unsigned long long noncePosition = nonce % staggersize;

	// Sort them using new distribution scheme:
	for(i = 0; i < SCOOPS; i++)
		memcpy( &cache[((i + SCOOPS - noncePosition) % SCOOPS) * staggersize * 16 + noncePosition * 16], &gendata[i * 16], 16);
}

void *work_i(void *x_void_ptr) {
	unsigned long long *x_ptr = (unsigned long long *)x_void_ptr;
	unsigned long long i = *x_ptr;

	unsigned int n;
        for(n=0; n<noncesperthread; n++) {
            if(selecttype == 1) {
                if (n + 4 < noncesperthread)
                {
                    mnonce(addr, i + n);
                    n += 3;
                } else
                   nonce(addr,i + n);
#ifdef AVX2
            } else if(selecttype == 2) {
                if (n + 8 < noncesperthread)
                {
                    m256nonce(addr, i + n);

                    n += 7;
                } else
                    nonce(addr, i + n);
#endif
            } else {
                nonce(addr, i + n);
            }
        }

	return NULL;
}

unsigned long long getMS() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}

void usage(char **argv) {
	printf("Usage: %s -k KEY [ -x CORE ] [-d DIRECTORY] [-s STARTBLOCK] [-n SIZE] [-m MEMORY] [-t THREADS]\n", argv[0]);
        printf("   CORE:\n");


	exit(-1);
}

void *writecache(void *arguments) {
	unsigned long long bytes = (unsigned long long) staggersize * PLOT_SIZE;
	unsigned long long position = 0;
	int percent;

	percent = (int)(100 * lastrun / nonces);
	unsigned long long ms = getMS() - starttime;

	if(asyncmode == 1) {
		printf("\33[2K\r%i percent done. (ASYNC write)", percent);
		fflush(stdout);
	} else {
		printf("\33[2K\r%i percent done. (write)", percent);
		fflush(stdout);
	}

	do {
		int b = write(ofd, &wcache[position], bytes > 100000000 ? 100000000 : bytes);	// Dont write more than 100MB at once
		position += b;
		bytes -= b;
	} while(bytes > 0);


	double minutes = (double)ms / (1000000 * 60);
	int speed = (int)(staggersize / minutes);
	int mb = speed / 60;
	int m = (int)(nonces) / speed;
	int h = (int)(m / 60);
	m -= h * 60;

	printf("\33[2K\r%i percent done. %i nonces/minute, %i MB/s, %i:%02i left", percent, speed, mb, h, m);
	fflush(stdout);

	return NULL;
}

int main(int argc, char **argv) {
	if(argc < 2) 
		usage(argv);

	int i;
	int startgiven = 0;
        for(i = 1; i < argc; i++) {
		// Ignore unknown argument
                if(argv[i][0] != '-')
			continue;

		if(argv[i][1] == 'a') {
			asyncmode = 1;
			continue;
		}

		char *parse = NULL;
		unsigned long long parsed;
		char param = argv[i][1];
		int modified, ds;

		if(argv[i][2] == 0) {
			if(i < argc - 1)
				parse = argv[++i];
		} else {
			parse = &(argv[i][2]);
		}
		if(parse != NULL) {
			modified = 0;
			parsed = strtoull(parse, 0, 10);
			switch(parse[strlen(parse) - 1]) {
				case 't':
				case 'T':
					parsed *= 1024;
				case 'g':
				case 'G':
					parsed *= 1024;
				case 'm':
				case 'M':
					parsed *= 1024;
				case 'k':
				case 'K':
					parsed *= 1024;
					modified = 1;
			}
			switch(param) {
				case 'k':
					addr = parsed;
					break;
				case 's':
					startnonce = parsed;
					startgiven = 1;
					break;
				case 'n':
					if(modified == 1) {
						nonces = (unsigned long long)(parsed / PLOT_SIZE);
					} else {
						nonces = parsed;
					}	
					break;
				case 'm':
					if(modified == 1) {
						staggersize = (unsigned long long)(parsed / PLOT_SIZE);
					} else {
						staggersize = parsed;
					}	
					break;
				case 't':
					threads = parsed;
					break;
				case 'x':
					selecttype = parsed;
					break;
				case 'd':
					ds = strlen(parse);
					outputdir = (char*) malloc(ds + 2);
					memcpy(outputdir, parse, ds);
					// Add final slash?
					if(outputdir[ds - 1] != '/') {
						outputdir[ds] = '/';
						outputdir[ds + 1] = 0;
					} else {
						outputdir[ds] = 0;
					}
					
			}			
		}
        }

        if(selecttype == 1) printf("Using SSE2 core.\n");

        else {
		printf("Using original algorithm.\n");
		selecttype=0;
	}

	if(addr == 0)
		usage(argv);


	// Autodetect threads
	if(threads == 0)
		threads = getNumberOfCores();

	// No startnonce given: Just pick random one
	if(startgiven == 0) {
		// Just some randomness
		srand(time(NULL));
		startnonce = (unsigned long long)rand() * (1 << 17) + rand();
	}

	// No nonces given: use whole disk
	if(nonces == 0) {
		unsigned long long fs = freespace(outputdir);
				
		nonces = (unsigned long long)(fs / PLOT_SIZE);
		if(nonces < 1) {
			printf("Not enough free space on device\n");
			exit(-1);
		}
	}

	// More memory than actual nonces? Reduce memory:
	if(staggersize > nonces) {
		staggersize = nonces;
	}

	// Autodetect stagger size
	if(staggersize == 0) // use 80% of memory
		staggersize = (freemem() * 0.8) / PLOT_SIZE;

	// Is asyncmode enabled? Only half the memory is available
	if(asyncmode == 1) 
		staggersize /= 2;

	if(staggersize < 1) 
		staggersize = 1;	// Needs at least 1 to make sense..

	// Optimum stagger size is 2 * SCOOPS. If we have more than 4 * SCOOPS we can enable Async Mode
	if(staggersize >= 4 * SCOOPS) asyncmode = 1;
	
	// Dont use more than optimum
	if(staggersize > 2 * SCOOPS)
		staggersize = 2 * SCOOPS;

	if(threads > 512) {
		// Limit threads to reasonable amount
		threads = 512;
		printf("Warning: Threads limited to 512.\n");
	}

	noncesperthread = (unsigned long)(staggersize / threads);

	// Too many threads? 
	if(noncesperthread == 0) {
		printf("Warning: Not enough memory for %i threads. Setting threads to %i\n", threads, staggersize);
		threads = staggersize;
		noncesperthread = 1;
	}

	unsigned long long iOffset = startnonce % SCOOPS;
	if(iOffset > 0) {
		startnonce -= iOffset;
		printf("Adjusting startnonce to %llu\n", startnonce);
	}

	printf("Creating plots for nonces %llu to %llu, %u GB using %u MB memory and %u threads\n",
		startnonce,
		(startnonce + nonces) - 1,
		PLOT_SIZE / 1024 * nonces / 1024 / 1024,
		PLOT_SIZE / 1024 * staggersize / 1024 * (1 + asyncmode),
		threads
	);

	if(asyncmode == 1) {
		acache[0] = calloc( PLOT_SIZE, staggersize);
		acache[1] = calloc( PLOT_SIZE, staggersize);

		if(acache[0] == NULL || acache[1] == NULL) {
			printf("Error allocating memory.\n");
			exit(-1);
		}
	} else {
		cache = calloc( PLOT_SIZE, staggersize );

		if(cache == NULL) {
			printf("Error allocating memory.\n");
			exit(-1);
		}
	}

	mkdir(outputdir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);

	char name[255];
	sprintf(name, "%s%llu_%llu_%u_v2", outputdir, addr, startnonce, staggersize);

	ofd = open(name, O_CREAT | O_LARGEFILE | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(ofd < 0) {
		printf("Error opening file %s\n", name);
		exit(0);
	}

	pthread_t worker[threads], writeworker;
	unsigned long long nonceoffset[threads];

	int asyncbuf = 0;
	unsigned long long astarttime;
	if(asyncmode == 1) cache=acache[asyncbuf];
	else wcache=cache;

	for(run = 0; run <= nonces - staggersize; run += staggersize) {
		astarttime = getMS();

		for(i = 0; i < threads; i++) {
			nonceoffset[i] = startnonce + i * noncesperthread;

			//printf("Starting thread %i, offset %llu, nonces: %u\n", i, nonceoffset[i], noncesperthread);
			if(pthread_create(&worker[i], NULL, work_i, &nonceoffset[i])) {
				printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
				exit(-1);
			}
		}

		// Wait for Threads to finish;
		for(i=0; i<threads; i++) {
			pthread_join(worker[i], NULL);
		}
		
		// Run leftover nonces
		for(i=threads * noncesperthread; i<staggersize; i++) {
			nonce(addr, startnonce + i);
		}

		// Write plot to disk:
		starttime=astarttime;
		if(asyncmode == 1) {
			if(run > 0) pthread_join(writeworker, NULL);
			lastrun = run + staggersize;
			wcache = cache;
			if(pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
				printf("Error creating thread. Out of memory? Try lower stagger size / less threads / remove async mode\n");
				exit(-1);
			}
			asyncbuf = 1 - asyncbuf;			
			cache = acache[ asyncbuf ];
		} else {
			lastrun = run + staggersize;
			if(pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
				printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
				exit(-1);
			}
			pthread_join(writeworker, NULL);
		}
		startnonce += staggersize;
	}
		
	if(asyncmode == 1) pthread_join(writeworker, NULL);

	close(ofd);

	printf("\nFinished plotting.\n");
	return 0;
}


