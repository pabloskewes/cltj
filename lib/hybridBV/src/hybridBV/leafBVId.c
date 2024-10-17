// supports leaf bitvectors of size up to 2^32-1

#include "hybridBV/leafBVId.h"

const int MaxBlockWords = 32;
const float NewFraction = 0.75; // new blocks try to be this fraction full

// size that a newly created leaf should have

extern inline uint leafBVIdNewSize(uint width) {
    return leafBVIdMaxSize(width) * NewFraction;
}

// max leaf size in elems
extern inline uint leafBVIdMaxSize(uint width) {
    return ((MaxBlockWords * w64)/width/2) * 2;
}

// data size in bytes
inline uint64_t bitmapSize(uint width) {
    return (leafBVIdMaxSize(width)+w64-1)/w64 * sizeof(uint64_t);
}

// id_data size in bytes
inline uint64_t sequenceSize(uint width) {
    return (leafBVIdMaxSize(width)*width+w64-1)/w64 * sizeof(uint64_t);
}

// creates an empty leafBV

leafBVId leafBVIdCreate(uint width) {
    leafBVId B = (leafBVId) myalloc(sizeof(struct s_leafBVId));
    B->size = 0;
    B->ones = 0;
    B->width = width;
    B->data = (uint64_t *) myalloc(bitmapSize(width));
    B->id_data = (uint64_t *) myalloc(sequenceSize(width));
    return B;
}

// converts a bit array into a leafBV of n bits
// data is copied but freed only if freeit

leafBVId leafBVIdCreateFrom64(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width, int freeit) {
    uint i, nb, p;
    leafBVId B;
    uint64_t word;

    if (n == 0) return leafBVIdCreate(width);
    B = (leafBVId) myalloc(sizeof(struct s_leafBVId));
    B->size = n;

    //bitvector
    B->data = (uint64_t *) myalloc(bitmapSize(width));
    nb = (n + 7) / 8;
    memcpy(B->data, bv_data, nb);
    nb = (n + w64 - 1) / w64;
    if (freeit) free(bv_data);
    if (n % w64) B->data[nb - 1] &= (((uint64_t) 1) << (n % w64)) - 1;
    B->ones = 0;
    for (i = 0; i < nb; i++) B->ones += popcount(B->data[i]);

    //sequence
    B->width = width;
    B->id_data = (uint64_t *) mycalloc(sequenceSize(width)/sizeof(uint64_t), sizeof(uint64_t));
    p = 0;
    for (i=0;i<n;i++)
    {   word = id_data[i];
        B->id_data[p/w64] |= word << (p%w64);
        if ((p%w64)+width > w64) B->id_data[p/w64+1] = word >> (w64-(p%w64));
        p += width;
    }
    if (freeit) free(id_data);
    return B;
}


leafBVId leafBVIdCreateFromPacked(uint64_t *bv_data, uint64_t *id_data, uint64_t i, uint64_t n, uint width, int freeit)

{  leafBVId B;
    if (n == 0) return leafBVIdCreate(width);
    B = (leafBVId)myalloc(sizeof(struct s_leafBVId));
    B->size = n;
    B->width = width;

    //bitvector
    B->data = (uint64_t *) myalloc(bitmapSize(width));
    uint nb = (n + 7) / 8;
    memcpy(B->data, bv_data, nb);
    nb = (n + w64 - 1) / w64;
    if (freeit) free(bv_data);
    if (n % w64) B->data[nb - 1] &= (((uint64_t) 1) << (n % w64)) - 1;
    B->ones = 0;
    for (uint k = 0; k < nb; k++) B->ones += popcount(B->data[k]);

    //sequence
    B->id_data = (uint64_t*)myalloc(sequenceSize(width));
    if (i == 0) memcpy(B->id_data, id_data,(n*width+7)/8);
    else copyBits(B->id_data,0,id_data,i*width,n*width);
    if(freeit) myfree(id_data);
    return B;
}


leafBVId leafBVIdClone(leafBVId B) {
    leafBVId BC = (leafBVId) myalloc(sizeof(struct s_leafBVId));
    BC->size = B->size;

    //bitvector
    BC->ones = B->ones;
    BC->data = (uint64_t *) myalloc(bitmapSize(B->width));
    if (BC->size) {
        memcpy(BC->data, B->data, (BC->size + 7) / 8);
    }

    //sequence
    BC->width = B->width;
    BC->id_data = (uint64_t *) myalloc(sequenceSize(B->width));
    if (BC->size) {
        memcpy(BC->id_data, B->id_data, (BC->size * BC->width + 7) / 8);
    }
    return BC;
}

// destroys B, frees data

void leafBVIdDestroy(leafBVId B) {
    myfree(B->data); //bitvector
    myfree(B->id_data); //sequence
    myfree(B);
}

// saves leaf data to file, which must be opened for writing

void leafBVIdSave(leafBVId B, FILE *file) {
    myfwrite(&B->size, sizeof(uint64_t), 1, file);
    myfwrite(&B->width, sizeof(byte), 1, file);
    if (B->size != 0) {
        myfwrite(B->data, sizeof(uint64_t), bitmapSize(B->width)/sizeof(uint64_t), file); //bitvector
        myfwrite(B->id_data, sizeof(uint64_t), sequenceSize(B->width)/sizeof(uint64_t), file); //sequence
    }
}

// loads leaf data from file, which must be opened for reading
// size is the number of bits

leafBVId leafBVIdLoad(FILE *file) {
    leafBVId B;
    uint64_t *bv_data, *id_data;
    uint64_t size;
    byte width;
    myfread(&size, sizeof(uint64_t), 1, file);
    myfread(&width, sizeof(byte), 1, file);

    bv_data = (uint64_t *) myalloc(((size + w64 - 1) / w64) * sizeof(uint64_t));
    myfread(bv_data, sizeof(uint64_t), (size + w64 - 1) / w64, file);

    id_data = (uint64_t *) myalloc(((size * width + w64 - 1) / w64) * sizeof(uint64_t));
    myfread(id_data, sizeof(uint64_t), (size * width + w64 - 1) / w64, file);

    B = leafBVIdCreateFromPacked(bv_data, id_data, 0, size, width, 1);
    return B;
}

// gives (allocated) space of B in w-bit words

uint leafBVIdSpace(leafBVId B) {
    return (sizeof(struct s_leafBVId) + sizeof(uint64_t) - 1) / sizeof(uint64_t)
           + ((bitmapSize(B->width)+sizeof(uint64_t)-1)/sizeof(uint64_t))
           + ((sequenceSize(B->width)+sizeof(uint64_t)-1)/sizeof(uint64_t));
}

// gives bit length

extern inline uint leafBVIdLength(leafBVId B) {
    return B->size;
}

// gives number of 1s

extern inline uint leafBVIdOnes(leafBVId B) {
    return B->ones;
}

// sets value for B[i]= (v != 0), assumes i is right
// returns difference in 1s

int leafBVIdWriteBV(leafBVId B, uint i, uint bv_v) {
    uint64_t one = ((uint64_t) 1) << (i % w64);
    if (bv_v) {
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

void leafBVIdWriteId(leafBVId B, uint i, uint64_t v) {
    uint64_t word;
    uint iq, ir;
    if (B->width == w64) {
        B->id_data[i] = v;
        return;
    }
    word = (((uint64_t) 1) << B->width) - 1;
    iq = i * B->width / w64;
    ir = (i * B->width) % w64;
    B->id_data[iq] &= ~(word << ir);
    B->id_data[iq] |= v << ir;
    if (ir + B->width > w64) {
        B->id_data[iq + 1] &= ~(word >> (w64 - ir));
        B->id_data[iq + 1] |= v >> (w64 - ir);
    }
}

// inserts v at B[i], assumes i is right and that insertion is possible

void leafBVIdInsertBV(leafBVId B, uint i, uint v) {
    uint nb = (B->size+1) / w64;
    uint ib = i / w64;
    int b;

    for (b = nb; b > ib; b--)
        B->data[b] = (B->data[b] << 1) | (B->data[b - 1] >> (w64 - 1));
    if ((i + 1) % w64)
        B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                      (((uint64_t) v) << (i % w64)) |
                      ((B->data[ib] << 1) & (~((uint64_t) 0) << ((i + 1) % w64)));
    else
        B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                      (((uint64_t) v) << (i % w64));
    B->ones += v;
}



void leafBVIdInsertId(leafBVId B, uint i, uint64_t v) {
    uint nb = (B->size+1) * B->width / w64;
    uint ib = i * B->width / w64;
    uint ir = (i * B->width) % w64;
    int b;

    if (B->width == w64) {
        for (b = nb; b > ib; b--) B->id_data[b] = B->id_data[b - 1];
        B->id_data[ib] = v;
        return;
    }
    for (b = nb; b > ib; b--)
        B->id_data[b] = (B->id_data[b] << B->width) |
                        (B->id_data[b - 1] >> (w64 - B->width));
    if (ir + B->width < w64)
        B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) | (v << ir) |
                         ((B->id_data[ib] << B->width) &
                          ((~(uint64_t) 0) << (ir + B->width)));
    else {
        B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) | (v << ir);
        B->id_data[ib + 1] = (B->id_data[ib + 1] & ((~(uint64_t) 0) << (ir + B->width - w64))) |
                             (v >> (w64 - ir));
    }
}

void leafBVIdInsert(leafBVId B, uint i, uint bv_v, uint64_t v, uint first) {

    //sequence
    uint nb = (B->size+1) * B->width / w64;
    uint ib = i * B->width / w64;
    uint ir = (i * B->width) % w64;
    int b;
    if (B->width == w64) {
        for (b = nb; b > ib; b--) B->id_data[b] = B->id_data[b - 1];
        B->id_data[ib] = v;
    }else {
        for (b = nb; b > ib; b--)
            B->id_data[b] = (B->id_data[b] << B->width) |
                            (B->id_data[b - 1] >> (w64 - B->width));
        if (ir + B->width < w64)
            B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) | (v << ir) |
                             ((B->id_data[ib] << B->width) &
                              ((~(uint64_t) 0) << (ir + B->width)));
        else {
            B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) | (v << ir);
            B->id_data[ib + 1] = (B->id_data[ib + 1] & ((~(uint64_t) 0) << (ir + B->width - w64))) |
                                 (v >> (w64 - ir));
        }
    }

    //bitvector
    nb = (B->size+1) / w64;
    ib = i / w64;
    for (b = nb; b > ib; b--)
        B->data[b] = (B->data[b] << 1) | (B->data[b - 1] >> (w64 - 1));
    if ((i + 1) % w64)
        B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                      (((uint64_t) bv_v) << (i % w64)) |
                      ((B->data[ib] << 1) & (~((uint64_t) 0) << ((i + 1) % w64)));
    else
        B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                      (((uint64_t) bv_v) << (i % w64));
    if(!bv_v && first) {
        B->data[ib] ^= (((uint64_t) 3) << (i % w64));
    }
    B->ones += bv_v;
    B->size++;
}

// deletes B[i], assumes i is right
// returns difference in 1s

int leafBVIdDeleteBV(leafBVId B, uint i) {
    uint nb = (B->size-1) / w64;
    uint ib = i / w64;
    int b;
    int v = (B->data[ib] >> (i % w64)) & 1;

    B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                  ((B->data[ib] >> 1) & (~((uint64_t) 0) << (i % w64)));
    for (b = ib + 1; b <= nb; b++) {
        B->data[b - 1] |= B->data[b] << (w64 - 1);
        B->data[b] >>= 1;
    }
    B->ones -= v;
    return -v;
}


void leafBVIdDeleteId(leafBVId B, uint i) {
    uint nb = B->size * B->width / w64;
    uint ib = i * B->width / w64;
    uint ir = (i * B->width) % w64;
    int b;

    if (B->width == w64) {
        for (b = ib + 1; b < nb; b++) B->id_data[b - 1] = B->id_data[b];
        return;
    }
    if (ir + B->width <= w64)
        B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) |
                         ((B->id_data[ib] >> B->width) & (~((uint64_t) 0) << ir));
    else {
        B->id_data[ib + 1] = (B->id_data[ib + 1] & (~(uint64_t) 0) << (ir + B->width - w64)) |
                             ((B->id_data[ib] >> (w64 - B->width)) &
                              ((((uint64_t) 1) << (ir + B->width - 1)) - 1));
        B->id_data[ib] &= (~(uint64_t) 0) << (w64 - B->width);
    }
    for (b = ib + 1; b <= nb; b++) {
        B->id_data[b - 1] |= B->id_data[b] << (w64 - B->width);
        B->id_data[b] >>= B->width;
    }
}

int leafBVIdDelete(leafBVId B, uint i, uint more, uint *nl) {
    //printf("leaf delete"); fflush(stdout);
    //sequence
    uint nb = B->size * B->width / w64;
    uint ib = i * B->width / w64;
    uint ir = (i * B->width) % w64;
    int b;

    if (B->width == w64) {
        for (b = ib + 1; b < nb; b++) B->id_data[b - 1] = B->id_data[b];
    }else {
        if (ir + B->width <= w64)
            B->id_data[ib] = (B->id_data[ib] & ((((uint64_t) 1) << ir) - 1)) |
                             ((B->id_data[ib] >> B->width) & (~((uint64_t) 0) << ir));
        else {
            B->id_data[ib + 1] = (B->id_data[ib + 1] & (~(uint64_t) 0) << (ir + B->width - w64)) |
                                 ((B->id_data[ib] >> (w64 - B->width)) &
                                  ((((uint64_t) 1) << (ir + B->width - 1)) - 1));
            B->id_data[ib] &= (~(uint64_t) 0) << (w64 - B->width);
        }
        for (b = ib + 1; b <= nb; b++) {
            B->id_data[b - 1] |= B->id_data[b] << (w64 - B->width);
            B->id_data[b] >>= B->width;
        }
    }

    //bitvectors
    nb = B->size / w64;
    ib = i / w64;
    int v = (B->data[ib] >> (i % w64)) & 1;

    B->data[ib] = (B->data[ib] & ((((uint64_t) 1) << (i % w64)) - 1)) |
                  ((B->data[ib] >> 1) & (~((uint64_t) 0) << (i % w64)));
    for (b = ib + 1; b <= nb; b++) {
        B->data[b - 1] |= B->data[b] << (w64 - 1);
        B->data[b] >>= 1;
    }
    *nl = 0;
    if(v && more) { //when it removes a 1-bit but it has more siblings
        *nl = (B->size-1 == i); //update next leaf with a 1-bit at the first postion
        if(!*nl) {
            B->data[ib] |= ((uint64_t) 1) << (i % w64); //update current leaf with the removed 1-bit
            v = 0;
        }
    }
    B->ones -= v;
    B->size--;
    return -v;
}

// access B[i], assumes i is right

extern inline uint leafBVIdAccessBV(leafBVId B, uint i) {
    return (B->data[i / w64] >> (i % w64)) & 1;
}

extern inline uint64_t leafBVIdAccessId(leafBVId B, uint i) {
    uint64_t word;
    if (B->width == w64) return B->id_data[i];
    uint iq = i * B->width / w64;
    uint ir = (i * B->width) % w64;
    word = (B->id_data[iq] >> ir);
    if (ir + B->width > w64) word |= B->id_data[iq + 1] << (w64 - ir);
    return word & ((((uint64_t) 1) << B->width) - 1);
}

// read bits [i..i+l-1], onto D[j...]

void leafBVIdReadBV(leafBVId B, uint i, uint l, uint64_t *D, uint64_t j) {
    copyBits(D, j, B->data, i, l);
}

void leafBVIdReadId(leafBVId B, uint i, uint l, uint64_t *D) {
    uint j, width, iq, ir;
    width = B->width;
    if (width == w64) {
        memcpy(D, B->id_data + i, l * sizeof(uint64_t));
        return;
    }
    iq = i * B->width / w64;
    ir = (i * B->width) % w64;
    for (j = 0; j < l; j++) {
        D[j] = B->id_data[iq] >> ir;
        if (ir + width >= w64) {
            iq++;
            D[j] |= B->id_data[iq] << (w64 - ir);
            ir += width - w64;
        } else ir += width;
        D[j] &= ((((uint64_t) 1) << width) - 1);
    }
}

// computes rank(B,i), zero-based, assumes i is right

uint leafBVIdRank(leafBVId B, uint i) {
    int p, ib;
    uint ones = 0;
    ib = ++i / w64;
    for (p = 0; p < ib; p++) ones += popcount(B->data[p]);
    if (i % w64) ones += popcount(B->data[p] & ((((uint64_t) 1) << (i % w64)) - 1));
    return ones;
}

// computes select_1(B,j), zero-based, assumes j is right

uint leafBVIdSelect(leafBVId B, uint j) {
    uint p, i, pc;
    uint64_t word;
    uint ones = 0;
    p = 0;
    while (1) {
        word = B->data[p];
        pc = popcount(word);
        if (ones + pc >= j) break;
        ones += pc;
        p++;
    }
    i = p * w64;
    while (1) {
        ones += word & 1;
        if (ones == j) return i;
        word >>= 1;
        i++;
    }
}

// trick for lowest 1 in a 64-bit word
static int decode[64] = {
    0, 1,56, 2,57,49,28, 3,61,58,42,50,38,29,17, 4,
   62,47,59,36,45,43,51,22,53,39,33,30,24,18,12, 5,
   63,55,48,27,60,41,37,16,46,35,44,21,52,32,23,11,
   54,26,40,15,34,20,31,10,25,14,19, 9,13, 8, 7, 6 };

int leafBVIdNext (leafBVId B, uint i)

{ uint p;
    uint64_t word;
    p = i/w64;
    word = B->data[p] & ((~(uint64_t)0)<<(i%w64));
    while ((++p*w64 <= B->size) && !word)
        word = B->data[p];
    if (p*w64 > B->size)
        word &= (((uint64_t)1) << (B->size % w64)) - 1;
    if (!word) return -1;
    return (p-1)*w64 + decode[(0x03f79d71b4ca8b09 * (word & -word))>>58];
}
