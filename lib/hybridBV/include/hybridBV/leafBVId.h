
#ifndef INCLUDEDleafBVId
#define INCLUDEDleafBVId

// supports leaf bitvectors of size up to 2^32-1

#include "basics.h"

typedef struct s_leafBVId {
  uint size;      // bits represented
  uint ones;      // # 1s
  uint64_t *data; // bitvector
  byte width;     // bits used per element, up to w
  uint64_t *id_data;
} *leafBVId;

// size that a newly created leaf should have, and max leaf size
// multiples of w
extern inline uint leafBVIdNewSize(uint width);
extern inline uint leafBVIdMaxSize(uint width);
extern inline uint64_t bitmapSize(uint width);
extern inline uint64_t sequenceSize(uint width);

// creates an empty leafBV
leafBVId leafBVIdCreate(uint width);

// converts a bit array into a leafBV of n bits
// data is copied but freed only if freeit
leafBVId leafBVIdCreateFrom64(
    uint64_t *bv_data,
    uint64_t *id_data,
    uint64_t n,
    uint width,
    int freeit
);
leafBVId leafBVIdCreateFromPacked(
    uint64_t *bv_data,
    uint64_t *id_data,
    uint64_t i,
    uint64_t n,
    uint width,
    int freeit
);

// destroys B, frees data
void leafBVIdDestroy(leafBVId B);

// saves leaf data to file, which must be opened for writing
void leafBVIdSave(leafBVId B, FILE *file);

// loads leaf data from file, which must be opened for reading
// size is the number of bits
leafBVId leafBVIdLoad(FILE *file);

// gives (allocated) space of B in w-bit words
uint leafBVIdSpace(leafBVId B);

// gives bit length
extern inline uint leafBVIdLength(leafBVId B);

// gives number of 1s
extern inline uint leafBVIdOnes(leafBVId B);

// sets value for B[i]= (v != 0), assumes i is right
// returns difference in 1s
int leafBVIdWriteBV(leafBVId B, uint i, uint v);
void leafBVIdWriteId(leafBVId B, uint i, uint64_t v);

// inserts v at B[i], assumes i is right and that insertion is possible
void leafBVIdInsertBV(leafBVId B, uint i, uint v);
void leafBVIdInsertId(leafBVId B, uint i, uint64_t v);
void leafBVIdInsert(leafBVId B, uint i, uint bv, uint64_t v, uint first);

// clones B
leafBVId leafBVIdClone(leafBVId B);

// deletes B[i], assumes i is right
// returns difference in 1s
int leafBVIdDeleteBV(leafBVId B, uint i);
void leafBVIdDeleteId(leafBVId B, uint i);
int leafBVIdDelete(leafBVId B, uint i, uint more, uint *nl);

// access B[i], assumes i is right
extern inline uint leafBVIdAccessBV(leafBVId B, uint i);
extern inline uint64_t leafBVIdAccessId(leafBVId B, uint i);

// read bits [i..i+l-1], onto D[j...]
void leafBVIdReadBV(leafBVId B, uint i, uint l, uint64_t *D, uint64_t j);
void leafBVIdReadId(leafBVId B, uint i, uint l, uint64_t *D);

// computes rank_1(B,i), zero-based, assumes i is right
uint leafBVIdRank(leafBVId B, uint i);

// computes select_1(B,j), zero-based, assumes j is right
uint leafBVIdSelect(leafBVId B, uint j);

int leafBVIdNext1(leafBVId B, uint i);

// finds first value >= c in [i..j], which must be increasing
// returns j+1 if not found
uint leafBVIdNext(leafBVId B, uint i, uint j, uint64_t c, uint64_t *value);

uint leafBVIdCheckOnes(leafBVId B);
#endif
