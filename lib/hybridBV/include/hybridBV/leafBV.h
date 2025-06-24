
#ifndef INCLUDEDleafBV
#define INCLUDEDleafBV

// supports leaf bitvectors of size up to 2^32-1

#include "basics.h"

typedef struct s_leafBV {
  uint size; // bits represented
  uint ones; // # 1s
  uint64_t *data;
} *leafBV;

// size that a newly created leaf should have, and max leaf size
// multiples of w
extern inline uint leafNewSize(void);
extern inline uint leafMaxSize(void);

// creates an empty leafBV
leafBV leafCreate(void);

// converts a bit array into a leafBV of n bits
// data is copied but freed only if freeit
leafBV leafCreateFrom(uint64_t *data, uint n, int freeit);

// destroys B, frees data
void leafDestroy(leafBV B);

// saves leaf data to file, which must be opened for writing
void leafSave(leafBV B, FILE *file);

// loads leaf data from file, which must be opened for reading
// size is the number of bits
leafBV leafLoad(FILE *file, uint size);

// gives (allocated) space of B in w-bit words
uint leafSpace(leafBV B);

// gives bit length
extern inline uint leafLength(leafBV B);

// gives number of 1s
extern inline uint leafOnes(leafBV B);

// sets value for B[i]= (v != 0), assumes i is right
// returns difference in 1s
int leafWrite(leafBV B, uint i, uint v);

// inserts v at B[i], assumes i is right and that insertion is possible
void leafInsert(leafBV B, uint i, uint v);

// clones B
leafBV leafClone(leafBV B);

// deletes B[i], assumes i is right
// returns difference in 1s
int leafDelete(leafBV B, uint i);

// access B[i], assumes i is right
extern inline uint leafAccess(leafBV B, uint i);

// read bits [i..i+l-1], onto D[j...]
void leafRead(leafBV B, uint i, uint l, uint64_t *D, uint64_t j);

// computes rank_1(B,i), zero-based, assumes i is right
uint leafRank(leafBV B, uint i);

// computes select_1(B,j), zero-based, assumes j is right
uint leafSelect(leafBV B, uint j);

#endif
