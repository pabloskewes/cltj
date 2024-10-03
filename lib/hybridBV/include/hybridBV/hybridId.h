
#ifndef INCLUDEDhybridId
#define INCLUDEDhybridId

	// supports hybrid arrays of size up to 2^64-1

#include "leafId.h"
#include "hybridBV.h" // to include nodeType

typedef struct s_hybridId *hybridId;

typedef struct s_dynamicId
   { uint64_t size;
     byte width; // up to w
     uint64_t leaves; // leaves below node
     uint64_t accesses; // since last update
     hybridId left,right; // hybridIds
   } *dynamicId;

typedef struct s_hybridId
   { nodeType type;
     union
      { leafId stat;
        leafId leaf;
        dynamicId dyn;
      } bv;
   } *hybridId;
      
extern float FactorId; // reconstruction factor

	// creates an empty hybridId, of width width
hybridId hybridIdCreate (uint width);


hybridId hybridIdCreateFrom64NoFree (uint64_t *data, uint64_t n, uint width);

	// converts an array of uint64_t into a hybridId of n elements
	// of width width. data is pointed to and will be freed 
hybridId hybridIdCreateFrom64 (uint64_t *data, uint64_t n, uint width);

	// converts an array of uint32_t into a hybridId of n elements
	// of width width. data is pointed to and will be freed 
hybridId hybridIdCreateFrom32 (uint32_t *data, uint64_t n, uint width);

	// destroys B, frees data 
void hybridIdDestroy (hybridId B);

// clones B
hybridId hybridIdClone (hybridId B);

	// writes B to file, which must be opened for writing
void hybridIdSave (hybridId B, FILE *file);

	// loads hybridId from file, which must be opened for reading
hybridId hybridIdLoad (FILE *file);

	// gives space of hybridId in w-bit words
uint64_t hybridIdSpace (hybridId B);

	// gives number of elements length
extern inline uint64_t hybridIdLength (hybridId B);

	// gives width of elements 
extern inline uint hybridIdWidth (hybridId B);

	// sets value for B[i] = v, assumes i is right and v fits in width
void hybridIdWrite (hybridId B, uint64_t i, uint64_t v);

	// inserts v at B[i], assumes i is right and v fits in width
void hybridIdInsert (hybridId B, uint64_t i, uint64_t v);

	// deletes B[i], assumes i is right
void hybridIdDelete (hybridId B, uint64_t i);

	// access B[i], assumes i is right
uint64_t hybridIdAccess (hybridId B, uint64_t i);

        // read values [i..i+l-1], onto D[0...], of uint64_t
void hybridIdRead64 (hybridId B, uint64_t i, uint64_t l, uint64_t *D);

        // read values [i..i+l-1], onto D[0...], of uint32_t
void hybridIdRead32 (hybridId B, uint64_t i, uint64_t l, uint32_t *D);

#endif
