
#ifndef INCLUDEDhybridBV
#define INCLUDEDhybridBV

	// supports hybrid bitvectors of size up to 2^64-1

#include "staticBV.h"
#include "leafBV.h"

typedef enum {
  tDynamic  = 1,
  tStatic = 2,
  tLeaf = 3
 } nodeType;

typedef struct s_hybridBV *hybridBV;

typedef struct s_dynamicBV
   { uint64_t size;
     uint64_t ones;
     uint64_t leaves;
     uint64_t accesses; // since last update
     hybridBV left,right; // hybridBVs
   } *dynamicBV;

typedef struct s_hybridBV
   { nodeType type;
     union
      { staticBV stat;
        leafBV leaf;
        dynamicBV dyn;
      } bv;
   } *hybridBV;
      
extern float FactorBV; // reconstruction factor

	// creates an empty hybridBV 
hybridBV hybridCreate (void);


hybridBV hybridCreateFromNoFree (uint64_t *data, uint64_t n);

	// converts a bit array into a hybridBV of n bits
	// data is pointed to and will be freed 
hybridBV hybridCreateFrom (uint64_t *data, uint64_t n);


//clones B
hybridBV hybridClone(hybridBV B);

	// destroys B, frees data 
void hybridDestroy (hybridBV B);

	// writes B to file, which must be opened for writing
void hybridSave (hybridBV B, FILE *file);

	// loads hybridBV from file, which must be opened for reading
hybridBV hybridLoad (FILE *file);

	// gives space of hybridBV in w-bit words
uint64_t hybridSpace (hybridBV B);

	// gives bit length
extern inline uint64_t hybridLength (hybridBV B);

	// gives number of leaves
extern inline uint64_t hybridLeaves (hybridBV B);

	// gives number of 1s
extern inline uint64_t hybridOnes (hybridBV B);

	// sets value for B[i]= (v != 0), assumes i is right
	// returns difference in 1s
int hybridWrite (hybridBV B, uint64_t i, uint v);

	// inserts v at B[i], assumes i is right
	// returns number of leaves created (could be negative)
void hybridInsert (hybridBV B, uint64_t i, uint v);

	// deletes B[i], assumes i is right
	// returns difference in 1s
int hybridDelete (hybridBV B, uint64_t i);

	// access B[i], assumes i is right
uint hybridAccess (hybridBV B, uint64_t i);

        // read bits [i..i+l-1], onto D[j...]
void hybridRead (hybridBV B, uint64_t i, uint64_t l, uint64_t *D, uint64_t j);

	// computes rank_1(B,i), zero-based, assumes i is right
uint64_t hybridRank (hybridBV B, uint64_t i);

	// computes select_1(B,j), zero-based, assumes j is right
uint64_t hybridSelect (hybridBV B, uint64_t j);

#endif
