/*
	A faster plot generator for burstcoin
	Author: Markus Tervooren <info@bchain.info>
	Burst: BURST-R5LP-KEL9-UYLG-GFG6T

	Implementation of Shabal is taken from:
	http://www.shabal.com/?p=198

	Usage: ./plot <public key> <start nonce> <nonces> <stagger size> <threads>
*/

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
#include "helper.h"

// Leave 5GB free space
#define FREE_SPACE	(unsigned long long)5 * 1000 * 1000 * 1000
#define DEFAULTDIR	"plots/"

// Not to be changed below this
#define PLOT_SIZE	(4096 * 64)
#define HASH_SIZE	32
#define HASH_CAP	4096

unsigned long long addr = 0;
unsigned long long startnonce = 0;
unsigned int nonces = 0;
unsigned int staggersize = 0;
unsigned int threads = 0;
unsigned int noncesperthread;

char *cache;
char *outputdir = DEFAULTDIR;

void nonce(unsigned long long int addr, unsigned long long int nonce, unsigned long long cachepos) {
	char final[32];
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
	
	shabal_init(&x, 256);
	shabal(&x, gendata, 16 + PLOT_SIZE);
	shabal_close(&x, 0, 0, final);


	// XOR with final
	unsigned long long *start = (unsigned long long*)gendata;
	unsigned long long *fint = (unsigned long long*)&final;

	for(i = 0; i < PLOT_SIZE; i += 32) {
		*start ^= fint[0]; start ++;
		*start ^= fint[1]; start ++;
		*start ^= fint[2]; start ++;
		*start ^= fint[3]; start ++;
	}

	// Sort them:
	for(i = 0; i < PLOT_SIZE; i+=64)
		memmove(&cache[cachepos * 64 + (unsigned long long)i * staggersize], &gendata[i], 64);
}

void *work_i(void *x_void_ptr) {
	unsigned long long *x_ptr = (unsigned long long *)x_void_ptr;
	unsigned long long i = *x_ptr;

	unsigned int n;
        for(n=0; n<noncesperthread; n++)
                nonce(addr,(i + n), (unsigned long long)(i - startnonce + n));

	return NULL;
}

unsigned long long getMS() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}

void usage(char **argv) {
	printf("Usage: %s -k KEY [-d DIRECTORY] [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] [-t THREADS]\n", argv[0]);
	exit(-1);
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
					parsed *= 1000;
				case 'g':
				case 'G':
					parsed *= 1000;
				case 'm':
				case 'M':
					parsed *= 1000;
				case 'k':
				case 'K':
					parsed *= 1000;
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

	if(addr == 0)
		usage(argv);

	// Autodetect threads
	if(threads == 0)
		threads = getNumberOfCores();

	// No startnonce given: Just pick random one
	if(startgiven == 0) {
		// Just some randomness
		srand(time(NULL));
		startnonce = (unsigned long long)rand() * (1 << 30) + rand();
	}

	// No nonces given: use whole disk
	if(nonces == 0) {
		unsigned long long fs = freespace(outputdir);
		if(fs <= FREE_SPACE) {
			printf("Not enough free space on device\n");
			exit(-1);
		}
		fs -= FREE_SPACE;
				
		nonces = (unsigned long long)(fs / PLOT_SIZE);
	}

	// Autodetect stagger size
	if(staggersize == 0) {
		// use 80% of memory
		unsigned long long memstag = (freemem() * 0.8) / PLOT_SIZE;

		if(nonces < memstag) {
			// Small stack: all at once
			staggersize = nonces;
		} else {
			// Determine stagger that (almost) fits nonces
			for(i = memstag; i >= 1000; i--) {
				if( (nonces % i) < 1000) {
					staggersize = i;
					nonces-= (nonces % i);
					i = 0;
				}
			}
		}
	}

	// 32 Bit and above 4GB?
	if( sizeof( void* ) < 8 ) {
		if( staggersize > 15000 ) {
			printf("Cant use stagger sizes above 15000 with 32-bit version\n");
			exit(-1);
		}
	}

	// Adjust according to stagger size
	if(nonces % staggersize != 0) {
		nonces -= nonces % staggersize;
		nonces += staggersize;
		printf("Adjusting total nonces to %u to match stagger size\n", nonces);
	}

	printf("Creating plots for nonces %llu to %llu (%u GB) using %u MB memory and %u threads\n", startnonce, (startnonce + nonces), (unsigned int)(nonces / 4 / 953), (unsigned int)(staggersize / 4), threads);

	// Comment this out/change it if you really want more than 200 Threads
	if(threads > 200) {
		printf("%u threads? Sure?\n", threads);
		exit(-1);
	}

	cache = calloc( PLOT_SIZE, staggersize );

	if(cache == NULL) {
		printf("Error allocating memory. Try lower stagger size.\n");
		exit(-1);
	}

	mkdir(outputdir, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);

	char name[100];
	sprintf(name, "%s%llu_%llu_%u_%u", outputdir, addr, startnonce, nonces, staggersize);

	int ofd = open(name, O_CREAT | O_LARGEFILE | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(ofd < 0) {
		printf("Error opening file %s\n", name);
		exit(0);
	}

	// Threads:
	noncesperthread = (unsigned long)(staggersize / threads);

	if(noncesperthread == 0) {
		threads = staggersize;
		noncesperthread = 1;
	}

	pthread_t worker[threads];
	unsigned long long nonceoffset[threads];

	int run;
	
	for(run = 0; run < nonces; run += staggersize) {
		unsigned long long starttime = getMS();
		for(i = 0; i < threads; i++) {
			nonceoffset[i] = startnonce + i * noncesperthread;

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
		for(i=threads * noncesperthread; i<staggersize; i++)
			nonce(addr, startnonce + i, (unsigned long long)i);

		// Write plot to disk:
		unsigned long long bytes = (unsigned long long) staggersize * PLOT_SIZE;
		unsigned long long position = 0;
		do {
			int b = write(ofd, &cache[position], bytes > 100000000 ? 100000000 : bytes);	// Dont write more than 100MB at once
			position += b;
			bytes -= b;
		} while(bytes > 0);

		unsigned long long ms = getMS() - starttime;
		
		int percent = (int)(100 * run / nonces);
		double minutes = (double)ms / (1000000 * 60);
		int speed = (int)(staggersize / minutes);
		int m = (int)(nonces - run) / speed;
		int h = (int)(m / 60);
		m -= h * 60;

		printf("\r%i Percent done. %i nonces/minute, %i:%02i left                ", percent, speed, h, m);
		fflush(stdout);

		startnonce += staggersize;
	}
	
	close(ofd);

	printf("\nFinished plotting.\n");
	return 0;
}


