
#ifndef INCLUDEDbasics
#define INCLUDEDbasics

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

typedef uint32_t uint;

#define w64 64
#define mymin(x, y) (((x) < (y)) ? (x) : (y))
#define mymax(x, y) (((x) > (y)) ? (x) : (y))

void *myalloc(size_t n);
void *mycalloc(size_t n, size_t s);
void *myrealloc(void *p, size_t n);
void myfree(void *p);

void myfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void myfwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);

// number of bits needed to represent n, gives 1 for n=0
uint numbits(uint n);

// counts # of 1s in y
inline uint popcount(uint64_t y);

// copies len bits starting at *src + psrc
// to tgt from bit position ptgt
// WARNING: leave at least one extra word to spare in tgt
void copyBits(
    uint64_t *tgt,
    uint64_t ptgt,
    uint64_t *src,
    uint64_t psrc,
    uint64_t len
);

#endif
