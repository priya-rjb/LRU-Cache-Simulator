#include "cachelab.h"
#include <stdint.h>
#include <getopt.h>
#include <ctype.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>

struct cache_line{
	char v;
	uint64_t t;
	int LRU;
};

struct cache_line** cache = NULL;

int vflag = 0;
int hflag = 0;
int s_value;
int S_value;
int  E_value;
int  b_value;
int block_B;
char* trace_file = NULL;
int globalLRU = 0;

int hit_count;
int miss_count;
int eviction_count;

void freeCacheMemory(){
	for(int i = 0; i < S_value; i++){
		free(cache[i]);
	}
	free(cache);
}

void initialize_cache(){
	cache = (struct cache_line**) malloc(S_value*sizeof(struct cache_line*));
	if (cache ==NULL){
		freeCacheMemory(cache);
		return;
	}
	for (int i = 0; i <S_value;i++){
		cache[i] = (struct cache_line*)malloc(E_value*sizeof(struct cache_line));
		if (cache[i]==NULL){
			freeCacheMemory();
		}
	}

	for (int i = 0; i < S_value;i++){
		//TODO:
		for (int j = 0; j <E_value;j++){
			cache[i][j].v = 0;
			cache[i][j].t = 0;
			cache[i][j].LRU = 0;
		}

	}
}

void printUsage(){
	printf ("Here's more info: \n");
	printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile> \n");
	printf ("-h: Optional help flag that prints usage info \n");
	printf ("-s <s>: Number of set index bits (S = 2^s is the number of sets \n");
	printf ("-s <s>: Number of set index bits S = 2^s is the number of sets \n");
	printf ("-b <b>: Number of block bits (B = 2^b is the block size) \n");
	printf ("-t <tracefile>: Name of the valgrind trace to replay \n");
}

void access_data(uint64_t address){
	//S 00600a60,4
	//mask
	int set_bit_mask = (1<<s_value)-1;
	int set_index = ((address >> b_value) & set_bit_mask);
	//Tag Bits:
	int tag_from_address = address >> (s_value +b_value);

	//for loop to iterate the cachelines in the set:
	for(int i = 0; i < E_value; i++){
		if ((cache[set_index][i].t) == tag_from_address && cache[set_index][i].v == 1){
			hit_count++;
			if(vflag){
				printf("hit ");
			}
			cache[set_index][i].LRU = globalLRU++;
			return;
		}
		
	}
	//cache
	miss_count++;
	if(vflag){
		printf("miss ");
	}

	//Misses:
	for(int i = 0; i < E_value; i++){
		if (cache[set_index][i].v == 0){
			cache[set_index][i].v = 1;
			cache[set_index][i].t = tag_from_address;
			cache[set_index][i].LRU = globalLRU++;
			return;
		}
	}

	eviction_count++;
	if(vflag){
		printf("eviction ");
	}

	//Find the cacheline in the current set with the lowest lru
	int min_lru = INT_MAX;
	for (int i = 0; i < E_value;i++){
		if (cache[set_index][i].LRU <= min_lru){
			min_lru = cache[set_index][i].LRU;
		}
	}
	//Eviction: Find the cacheline with the lowest lru again to update it's fields:
	for(int i = 0; i <E_value;i++){
		if (cache[set_index][i].LRU == min_lru){
			cache[set_index][i].v = 1;
			cache[set_index][i].t = tag_from_address;
			cache[set_index][i].LRU = globalLRU++;
			return;
		}
	}

}

void read_from_file(char*trace_file){
	//char operation;
	uint64_t address;
	int size;

	//read the trace file:
	FILE *fp; //
	fp = fopen(trace_file, "r");

	if (fp == NULL) {
		perror("file open failed");
		exit(1);
	}
	int max_str_size = 80;
	char str[max_str_size];
	while (!feof(fp)){
		if (fgets(str, max_str_size, fp) != NULL) {
			if (str[1] == 'S' || str[1] == 'M' || str[1] == 'L' ){
				sscanf(str+3, "%"SCNx64", %u", &address, &size);
				
				if(vflag){
					printf("%c %"PRIx64", %d ", str[1], address, size);
				}
				access_data(address);

				if (str[1] == 'M'){
					access_data(address);
				}
				if(vflag){
					printf("\n");
				}
			}
		}
	}
	fclose(fp);
}

int main(int argc, char**argv) {
    int	c;

	while	((c	=	getopt(argc,argv,	"hvs:E:b:t:"))	!=	-1)	{
		switch	(c) {	
			case 'h':
			//TODO:
					hflag =1;
					printUsage();
					exit(0);
			case 'v':
			//TODO:
					vflag =1;
					break;
			case 's':
					s_value = atoi(optarg);
					S_value = pow(2, s_value);
					break;
			case 'E':
					E_value = atoi(optarg);
					break;
			case 'b':
					b_value = atoi(optarg);
					block_B = pow(2, b_value);
					break;
			case 't':
					trace_file = optarg;
					break; 
			default:
					abort();	//do	something
			}
    
	}
	if (s_value == 0|| E_value == 0|| b_value == 0 || trace_file == NULL){
		printUsage();
		exit(1);
	}
	initialize_cache();
	read_from_file(trace_file);
	printSummary(hit_count, miss_count, eviction_count);
	freeCacheMemory();
    return 0;
}