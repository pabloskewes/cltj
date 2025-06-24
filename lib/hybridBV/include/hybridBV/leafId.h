
#ifndef INCLUDEDleafId
#define INCLUDEDleafId

// supports leaf arrays of size up to 2^32-1

#include "basics.h"

typedef struct s_leafId {
  uint64_t size; // elements represented
  byte width;    // bits used per element, up to w
  byte isStat;   // does not accept indels
  uint64_t *data;
} *leafId;

// size that a newly created leaf should have, and max leaf size
// measured in elements
extern inline uint leafIdNewSize(uint width);
extern inline uint leafIdMaxSize(uint width);

// creates an empty leafId with elements of width width
leafId leafIdCreate(uint width);

// creates a static leafId of n elements of width width
// from data in an already packed array, which is simply pointed
leafId leafIdCreateStaticFromPacked(uint64_t *data, uint n, uint width);

// creates a dynamic leafId of n elements of width width
// from data[i..] in an already packed array, which is not destroyed
leafId leafIdCreateFromPacked(uint64_t *data, uint64_t i, uint n, uint width);

// converts an array of n uint64_t into a leafId of n elements
// of width width.
leafId
leafIdCreateFrom64NoFree(uint64_t *data, uint n, uint width, uint isStat);

// converts an array of n uint64_t into a leafId of n elements
// of width width. data is pointed to and will be freed
leafId leafIdCreateFrom64(uint64_t *data, uint n, uint width, uint isStat);

// converts an array of n uint32_t into a leafId of n elements
// of width width. data is pointed to and will be freed
leafId leafIdCreateFrom32(uint32_t *data, uint n, uint width, uint isStat);

// destroys B, frees data
void leafIdDestroy(leafId B);

// destroys B, frees data
leafId leafIdClone(leafId B);

// saves leaf data to file, which must be opened for writing
void leafIdSave(leafId B, FILE *file);

// loads leaf data from file, which must be opened for reading
leafId leafIdLoad(FILE *file);

// gives (allocated) space of B in w-bit words
uint leafIdSpace(leafId B);

// gives array length
extern inline uint leafIdLength(leafId B);

// sets value for B[i] = v
// assumes not static, i is right, and value fits in width
void leafIdWrite(leafId B, uint i, uint64_t v);

// inserts v at B[i], assumes that insertion is possible
// assumes not static, i is right, and value fits in width
void leafIdInsert(leafId B, uint i, uint64_t v);

// deletes B[i], assumes i is right
// assumes not static, i is right, and value fits in width
void leafIdDelete(leafId B, uint i);

// access B[i], assumes i is right
extern inline uint64_t leafIdAccess(leafId B, uint i);

// read bits [i..i+l-1], onto D[0...] of 64 bits
void leafIdRead64(leafId B, uint i, uint l, uint64_t *D);

// read bits [i..i+l-1], onto D[0...] of 32 bits
void leafIdRead32(leafId B, uint i, uint l, uint32_t *D);

#endif
