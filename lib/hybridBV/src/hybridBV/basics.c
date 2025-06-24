
#include "hybridBV/basics.h"

void *myalloc(size_t n)

{
  void *p;
  if (n == 0)
    return NULL;
  p = malloc(n);
  if (p == NULL) {
    fprintf(stderr, "Error: malloc of %li bytes returned null\n", n);
    exit(1);
  }
  return p;
}

void *mycalloc(size_t n, size_t s)

{
  void *p;
  if (n == 0)
    return NULL;
  p = calloc(n, s);
  if (p == NULL) {
    fprintf(stderr, "Error: calloc of %li bytes returned null\n", n);
    exit(1);
  }
  return p;
}

void *myrealloc(void *p, size_t n)

{
  if (p == NULL)
    return myalloc(n);
  if (n == 0)
    return NULL;
  p = realloc(p, n);
  if (p == NULL) {
    fprintf(stderr, "Error: realloc of %li bytes returned null\n", n);
    exit(1);
  }
  return p;
}

void myfree(void *p)

{
  if (p != NULL)
    free(p);
}

void myfread(void *ptr, size_t size, size_t nmemb, FILE *stream)

{
  if (fread(ptr, size, nmemb, stream) != nmemb) {
    fprintf(stderr, "Error: fread of %li bytes failed\n", nmemb * size);
    exit(1);
  }
}

void myfwrite(void *ptr, size_t size, size_t nmemb, FILE *stream)

{
  if (fwrite(ptr, size, nmemb, stream) != nmemb) {
    fprintf(stderr, "Error: fwrite of %li bytes failed\n", nmemb * size);
    exit(1);
  }
}

uint numbits(uint n)

{
  uint bits = 0;
  while (n) {
    n = n >> 1;
    bits++;
  }
  return bits ? bits : 1;
}

extern inline uint popcount(uint64_t y)

{
  y -= ((y >> 1) & 0x5555555555555555ull);
  y = (y & 0x3333333333333333ull) + (y >> 2 & 0x3333333333333333ull);
  return ((y + (y >> 4)) & 0xf0f0f0f0f0f0f0full) * 0x101010101010101ull >> 56;
}

// copies len bits starting at *src + psrc
// to tgt from bit position ptgt
// WARNING: writes some extra bits after target (but not more words)

void copyBits(
    uint64_t *tgt,
    uint64_t ptgt,
    uint64_t *src,
    uint64_t psrc,
    uint64_t len
)

{
  uint64_t old, mask;

  // easy cases if they are similarly misaligned
  // I didn't align to 8 due to possible endianness issues
  tgt += ptgt / w64;
  ptgt %= w64;
  src += psrc / w64;
  psrc %= w64;
  mask = (((uint64_t)1) << ptgt) - 1;
  if (ptgt == psrc) {
    if (ptgt != 0) {
      *tgt = (*tgt & mask) + (*src & ~mask);
      *tgt++;
      *src++;
      len -= w64 - ptgt;
    }
    memcpy(tgt, src, ((len + w64 - 1) / w64) * sizeof(uint64_t));
    return;
  }
  // general case, we first align the source
  if (ptgt < psrc) // first word from src fits in ptgt
  {
    *tgt = (*tgt & mask) + ((*src++ >> (psrc - ptgt)) & ~mask);
    ptgt += w64 - psrc;
  } else {
    *tgt = (*tgt & mask) + ((*src << (ptgt - psrc)) & ~mask);
    if (len <= w64 - ptgt)
      return; // before overflowing to the next word
    ptgt -= psrc;
    *++tgt = *src++ >> (w64 - ptgt);
  }
  if (len <= w64 - psrc)
    return;
  len -= w64 - psrc;
  // now src is aligned, copy all the rest
  mask = (((uint64_t)1) << ptgt) - 1;
  old = *tgt & mask;
  len += w64; // cannot write len >= 0 as it is unsigned
  while (len > w64) {
    *tgt++ = old + (*src << ptgt);
    old = *src++ >> (w64 - ptgt);
    len -= w64;
  }
  if (len + ptgt > w64)
    *tgt = old;
}
