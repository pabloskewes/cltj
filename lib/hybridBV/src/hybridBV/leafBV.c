
// supports leaf bitvectors of size up to 2^32-1

#include "hybridBV/leafBV.h"

#define MaxBlockWords 32 // measured in uint64_t's
#define NewFraction 0.75 // new blocks try to be this fraction full

// size that a newly created leaf should have

extern inline uint leafNewSize(void)

{
  return MaxBlockWords * NewFraction;
}

// max leaf size

extern inline uint leafMaxSize(void)

{
  return MaxBlockWords;
}

// creates an empty leafBV

leafBV leafCreate(void)

{
  leafBV B = (leafBV)myalloc(sizeof(struct s_leafBV));
  B->size = 0;
  B->ones = 0;
  B->data = (uint64_t *)myalloc(MaxBlockWords * sizeof(uint64_t));
  return B;
}

// converts a bit array into a leafBV of n bits
// data is copied but freed only if freeit

leafBV leafCreateFrom(uint64_t *data, uint n, int freeit)

{
  uint i, nb;
  leafBV B;
  if (n == 0)
    return leafCreate();
  B = (leafBV)myalloc(sizeof(struct s_leafBV));
  B->size = n;
  B->data = (uint64_t *)myalloc(MaxBlockWords * sizeof(uint64_t));
  nb = (n + 7) / 8;
  memcpy(B->data, data, nb);
  nb = (n + w64 - 1) / w64;
  if (freeit)
    free(data);
  if (n % w64)
    B->data[nb - 1] &= (((uint64_t)1) << (n % w64)) - 1;
  B->ones = 0;
  for (i = 0; i < nb; i++)
    B->ones += popcount(B->data[i]);
  return B;
}

leafBV leafClone(leafBV B) {
  leafBV BC = (leafBV)myalloc(sizeof(struct s_leafBV));
  BC->size = B->size;
  BC->ones = B->ones;
  BC->data = (uint64_t *)myalloc(MaxBlockWords * sizeof(uint64_t));
  if (BC->size) {
    memcpy(BC->data, B->data, (BC->size + 7) / 8);
  }
  return BC;
}
// destroys B, frees data

void leafDestroy(leafBV B)

{
  myfree(B->data);
  myfree(B);
}

// saves leaf data to file, which must be opened for writing

void leafSave(leafBV B, FILE *file)

{
  if (B->size != 0)
    myfwrite(B->data, sizeof(uint64_t), (B->size + w64 - 1) / w64, file);
}

// loads leaf data from file, which must be opened for reading
// size is the number of bits

leafBV leafLoad(FILE *file, uint size)

{
  uint64_t *data =
      (uint64_t *)myalloc(((size + w64 - 1) / w64) * sizeof(uint64_t));
  myfread(data, sizeof(uint64_t), (size + w64 - 1) / w64, file);
  return leafCreateFrom(data, size, 1);
}

// gives (allocated) space of B in w-bit words

uint leafSpace(leafBV B)

{
  return (sizeof(struct s_leafBV) + sizeof(uint64_t) - 1) / sizeof(uint64_t) +
         MaxBlockWords;
}

// gives bit length

extern inline uint leafLength(leafBV B)

{
  return B->size;
}

// gives number of 1s

extern inline uint leafOnes(leafBV B)

{
  return B->ones;
}

// sets value for B[i]= (v != 0), assumes i is right
// returns difference in 1s

int leafWrite(leafBV B, uint i, uint v)

{
  uint64_t one = ((uint64_t)1) << (i % w64);
  if (v) {
    if (!(B->data[i / w64] & one)) {
      B->data[i / w64] |= one;
      B->ones++;
      return 1;
    }
  } else {
    if (B->data[i / w64] & one) {
      B->data[i / w64] &= ~one;
      B->ones--;
      return -1;
    }
  }
  return 0;
}

// inserts v at B[i], assumes i is right and that insertion is possible

void leafInsert(leafBV B, uint i, uint v)

{
  uint nb = ++B->size / w64;
  uint ib = i / w64;
  int b;

  for (b = nb; b > ib; b--)
    B->data[b] = (B->data[b] << 1) | (B->data[b - 1] >> (w64 - 1));
  if ((i + 1) % w64)
    B->data[ib] = (B->data[ib] & ((((uint64_t)1) << (i % w64)) - 1)) |
                  (((uint64_t)v) << (i % w64)) |
                  ((B->data[ib] << 1) & (~((uint64_t)0) << ((i + 1) % w64)));
  else
    B->data[ib] = (B->data[ib] & ((((uint64_t)1) << (i % w64)) - 1)) |
                  (((uint64_t)v) << (i % w64));
  B->ones += v;
}

// deletes B[i], assumes i is right
// returns difference in 1s

int leafDelete(leafBV B, uint i)

{
  uint nb = B->size-- / w64;
  uint ib = i / w64;
  int b;
  int v = (B->data[ib] >> (i % w64)) & 1;

  B->data[ib] = (B->data[ib] & ((((uint64_t)1) << (i % w64)) - 1)) |
                ((B->data[ib] >> 1) & (~((uint64_t)0) << (i % w64)));
  for (b = ib + 1; b <= nb; b++) {
    B->data[b - 1] |= B->data[b] << (w64 - 1);
    B->data[b] >>= 1;
  }
  B->ones -= v;
  return -v;
}

// access B[i], assumes i is right

extern inline uint leafAccess(leafBV B, uint i)

{
  return (B->data[i / w64] >> (i % w64)) & 1;
}

// read bits [i..i+l-1], onto D[j...]

void leafRead(leafBV B, uint i, uint l, uint64_t *D, uint64_t j)

{
  copyBits(D, j, B->data, i, l);
}

// computes rank(B,i), zero-based, assumes i is right

uint leafRank(leafBV B, uint i)

{
  int p, ib;
  uint ones = 0;
  ib = ++i / w64;
  for (p = 0; p < ib; p++)
    ones += popcount(B->data[p]);
  if (i % w64)
    ones += popcount(B->data[p] & ((((uint64_t)1) << (i % w64)) - 1));
  return ones;
}

// computes select_1(B,j), zero-based, assumes j is right

uint leafSelect(leafBV B, uint j)

{
  uint p, i, pc;
  uint64_t word;
  uint ones = 0;
  p = 0;
  while (1) {
    word = B->data[p];
    pc = popcount(word);
    if (ones + pc >= j)
      break;
    ones += pc;
    p++;
  }
  i = p * w64;
  while (1) {
    ones += word & 1;
    if (ones == j)
      return i;
    word >>= 1;
    i++;
  }
}
