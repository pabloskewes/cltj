//
// Created by adrian on 10/10/24.
//

#ifndef HYBRIDBVID_H
#define HYBRIDBVID_H

#include "hybridBV.h" // to include nodeType
#include "leafBVId.h"
#include "staticBVId.h"

typedef struct s_hybridBVId *hybridBVId;

typedef struct s_dynamicBVId
{   uint64_t size;
    uint64_t ones;
    byte width; // up to w
    uint64_t last; // last value in subtree
    uint64_t leaves; // leaves below node
    uint64_t accesses; // since last update
    hybridBVId left,right; // hybridIds
} *dynamicBVId;

typedef struct s_hybridBVId
{ nodeType type;
    union
    {   staticBVId stat;
        leafBVId leaf;
        dynamicBVId dyn;
    } bv;
} *hybridBVId;

// creates an empty hybridLOUDS, whose sequence has width width
hybridBVId hybridBVIdCreate(uint width);

// creates a hybridBVId from id
// of width width.
//hybridBVId hybridBVIdCreateNoFree(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width);

// converts an array of uint64_t into a hybridId of n elements
// of width width. frees data.
hybridBVId hybridBVIdCreateFrom64(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width);
hybridBVId hybridBVIdCreateFromPacked(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width);

// destroys B, frees data
void hybridBVIdDestroy (hybridBVId B);

// clones B
hybridBVId hybridBVIdClone (hybridBVId B);

// writes B to file, which must be opened for writing
void hybridBVIdSave (hybridBVId B, FILE *file);

// loads hybridId from file, which must be opened for reading
hybridBVId hybridBVIdLoad (FILE *file);

// gives space of hybridId in w-bit words
uint64_t hybridBVIdSpace (hybridBVId B);

// gives number of elements length
extern inline uint64_t hybridBVIdLength (hybridBVId B);

// gives width of elements
extern inline uint hybridBVIdWidth (hybridBVId B);

// gives number of 1s
extern inline uint64_t hybridBVIdOnes (hybridBVId B);

// sets value for B[i] = v, assumes i is right and v fits in width
int hybridBVIdWriteBV (hybridBVId B, uint64_t i, uint64_t v);

// inserts v at B[i], assumes i is right and v fits in width
void hybridBVIdInsert (hybridBVId B, uint64_t i, uint bv_v, uint64_t id_v, uint first);

// deletes B[i], assumes i is right
int hybridBVIdDelete (hybridBVId B, uint64_t i, uint more);

// access B[i], assumes i is right
uint64_t hybridBVIdAccess (hybridBVId B, uint64_t i, uint64_t *id);

// access B[i], assumes i is right
uint64_t hybridBVIdAccessId (hybridBVId B, uint64_t i);

// computes rank_1(B,i), zero-based, assumes i is right
uint64_t hybridBVIdRank (hybridBVId B, uint64_t i);

// computes select_1(B,j), zero-based, assumes j is right
uint64_t hybridBVIdSelect (hybridBVId B, uint64_t j);

uint checkOnes(hybridBVId B);

uint checkLast(hybridBVId B);

uint checkOnesPrint(hybridBVId B);

uint checkLastPrint(hybridBVId B);

uint checkLeaves(hybridBVId B);

int64_t hybridBVIdNext1 (hybridBVId B, uint64_t i);

void hybridBVIdSplitMax(hybridBVId B);

void hybridBVIdFlatten(hybridBVId B);

// finds first value >= c in [i..j], which must be increasing
// returns j+1 if not found

uint64_t hybridBVIdNext (hybridBVId B, uint64_t i, uint64_t j, uint64_t c, uint64_t* value);

#endif //HYBRIDLOUDS_H
