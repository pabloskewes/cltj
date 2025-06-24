
#ifndef INCLUDEDstaticBVId
#define INCLUDEDstaticBVId

// supports static bitvectors of size up to 2^64-1

#include "basics.h"

typedef struct s_staticBVId {
  uint64_t size;  // number of bits
  uint64_t ones;  // number of 1s
  uint64_t *data; // the bits
  byte width;     // bits used per element, up to w
  uint64_t *id_data;
  uint64_t *S; // superblocks
  uint16_t *B; // blocks
} *staticBVId;

// converts a bit array into a bitvector of n bits
// data is pointed to and will be freed
staticBVId staticBVIdCreateFrom64(
    uint64_t *bv_data,
    uint64_t *id_data,
    uint64_t n,
    uint width
);
staticBVId staticBVIdCreateFromPacked(
    uint64_t *bv_data,
    uint64_t *id_data,
    uint64_t n,
    uint width
);

// destroys B, frees data
void staticBVIdDestroy(staticBVId B);

// writes B's data to file, which must be opened for writing
void staticBVIdSave(staticBVId B, FILE *file);

// loads bitvector's data from file, which must be opened for reading
// size is the number of bits
staticBVId staticBVIdLoad(FILE *file);

// clones B
staticBVId staticBVIdClone(staticBVId B);

// gives space of bitvector in w-bit words
uint64_t staticBVIdSpace(staticBVId B);

// data of staticBV
extern inline uint64_t *staticBVIdBits(staticBVId B);
extern inline uint64_t *staticBVIdIds(staticBVId B);

// gives bit length
extern inline uint64_t staticBVIdLength(staticBVId B);

// gives number of ones
extern inline uint64_t staticBVIdOnes(staticBVId B);

// access B[i], assumes i is right
extern inline uint staticBVIdAccessBV(staticBVId B, uint64_t i);
extern inline uint64_t staticBVIdAccessId(staticBVId B, uint64_t i);

// read bits [i..i+l-1], writes onto D[j..]
extern inline void
staticBVIdReadBV(staticBVId B, uint64_t i, uint64_t l, uint64_t *D, uint64_t j);
void staticBVIdReadId(staticBVId B, uint i, uint l, uint64_t *D);

// computes rank_1(B,i), zero-based, assumes i is right
extern inline uint64_t staticBVIdRank(staticBVId B, uint64_t i);

// computes select_1(B,j), zero-based, assumes j is right
extern uint64_t staticBVIdSelect(staticBVId B, uint64_t j);

int64_t staticBVIdNext1(staticBVId B, uint64_t i);

uint64_t staticBVIdNext(
    staticBVId B,
    uint64_t i,
    uint64_t j,
    uint64_t c,
    uint64_t *value
);
uint staticBVIdCheckOnes(staticBVId B);

#endif
