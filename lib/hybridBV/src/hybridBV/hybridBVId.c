// supports hybrid arrays of size up to 2^64-1

#include "hybridBV/hybridBVId.h"

//extern float FactorId = 0.01; // Factor * length reads => rebuild as static
extern float FactorId = 0.001; // Factor * length reads => rebuild as static

static inline int mustFlatten(hybridBVId B) {
    return (B->bv.dyn->accesses >= FactorId * B->bv.dyn->size);
}

static const float TrfFactor = 0.125; // TrfFactor * MaxLeafSize to justify transferLeft/Right

static const float AlphaFactor = 0.65; // balance factor 3/5 < . < 1

static const int MinLeavesToBalance = 5; // min number of leaves to balance the tree

static const float MinFillFactor = 0.3; // less than this involves rebuild.
// Must be <= NewFraction/2

// creates an empty hybridId

hybridBVId hybridBVIdCreate(uint width) {
    hybridBVId B = myalloc(sizeof(struct s_hybridBVId));
    B->type = tLeaf;
    B->bv.leaf = leafBVIdCreate(width);
    return B;
}

// converts an array of uint64_t into a hybridId of n elements
// of width width

hybridBVId hybridBVIdCreateFrom64(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width) {
    hybridBVId B = myalloc(sizeof(struct s_hybridBVId));
    if (n > leafBVIdNewSize(width)) {
        B->type = tStatic;
        B->bv.stat = staticBVIdCreateFrom64(bv_data, id_data, n, width);
    } else {
        B->type = tLeaf;
        B->bv.leaf = leafBVIdCreateFrom64(bv_data, id_data, n, width, 0);
    }
    return B;
}


hybridBVId hybridBVIdCreateFromPacked(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width) {
    hybridBVId B = myalloc(sizeof(struct s_hybridBVId));
    if (n > leafBVIdNewSize(width)) {
        B->type = tStatic;
        B->bv.stat = staticBVIdCreateFromPacked(bv_data, id_data, n, width);
    } else {
        B->type = tLeaf;
        B->bv.leaf = leafBVIdCreateFromPacked(bv_data, id_data, 0, n, width, 1);
    }
    return B;
}


hybridBVId hybridBVIdClone(hybridBVId B) {
    hybridBVId BC = myalloc(sizeof(struct s_hybridBVId));
    BC->type = B->type;
    if (B->type == tLeaf) BC->bv.leaf = leafBVIdClone(B->bv.leaf);
    else if (B->type == tStatic) BC->bv.stat = staticBVIdClone(B->bv.stat);
    else {
        BC->bv.dyn->ones = B->bv.dyn->ones;
        BC->bv.dyn->accesses = B->bv.dyn->accesses;
        BC->bv.dyn->leaves = B->bv.dyn->leaves;
        BC->bv.dyn->last = B->bv.dyn->last;
        BC->bv.dyn->size = B->bv.dyn->size;
        BC->bv.dyn->width = B->bv.dyn->width;
        BC->bv.dyn->left = hybridBVIdClone(B->bv.dyn->left);
        BC->bv.dyn->right = hybridBVIdClone(B->bv.dyn->right);
    }
    return BC;
}

// destroys B, frees data

void hybridBVIdDestroy(hybridBVId B) {
    if (B->type == tLeaf) leafBVIdDestroy(B->bv.leaf);
    else if (B->type == tStatic) staticBVIdDestroy(B->bv.stat);
    else {
        hybridBVIdDestroy(B->bv.dyn->left);
        hybridBVIdDestroy(B->bv.dyn->right);
        myfree(B->bv.dyn);
    }
    myfree(B);
}

// creates a static version of B, rewriting it but not its address

// reads B into D[j..] without unpacking

/*static void packedReadoold (hybridBVId B, uint64_t *bv_D, uint64_t *id_D, uint64_t j)

{ uint64_t lsize;
    uint width;
    if (B->type == tDynamic)
    { lsize = hybridBVIdLength(B->bv.dyn->left);
        packedRead(B->bv.dyn->left,bv_D,id_D,j);
        packedRead(B->bv.dyn->right,bv_D,id_D,j+lsize);
        return;
    }
    width = hybridBVIdWidth(B);
    if (B->type == tStatic) {
        copyBits(bv_D,j,B->bv.stat->data,0,hybridBVIdLength(B));
        copyBits(id_D, j*width, B->bv.stat->id_data, 0, hybridBVIdLength(B)*width);
    }else {
        copyBits(bv_D,j,B->bv.leaf->data,0,hybridBVIdLength(B));
        copyBits(id_D, j*width, B->bv.leaf->id_data, 0, hybridBVIdLength(B)*width);
    }
}*/

static void packedRead(hybridBVId B, uint64_t *D, uint64_t j) {
    uint64_t lsize;
    uint width;
    if (B->type == tDynamic) {
        lsize = hybridBVIdLength(B->bv.dyn->left);
        packedRead(B->bv.dyn->left, D, j);
        packedRead(B->bv.dyn->right, D, j + lsize);
        return;
    }
    width = hybridBVIdWidth(B);
    if (B->type == tStatic)
        copyBits(D, j * width, B->bv.stat->id_data, 0, hybridBVIdLength(B) * width);
    else copyBits(D, j * width, B->bv.leaf->id_data, 0, hybridBVIdLength(B) * width);
}

static void myread(hybridBVId B, uint64_t i, uint64_t l, uint64_t *D, uint64_t j) {
    uint64_t lsize;
    if (B->type == tLeaf) {
        copyBits(D, j, B->bv.leaf->data, i, l);
        return;
    }
    if (B->type == tDynamic) {
        lsize = hybridBVIdLength(B->bv.dyn->left);
        if (i + l < lsize) myread(B->bv.dyn->left, i, l, D, j);
        else if (i >= lsize) myread(B->bv.dyn->right, i - lsize, l, D, j);
        else {
            myread(B->bv.dyn->left, i, lsize - i, D, j);
            myread(B->bv.dyn->right, 0, l - (lsize - i), D, j + (lsize - i));
        }
        return;
    }
    copyBits(D, j, B->bv.stat->data, i, l);
}


// collects all the descending elements into an array, destroys bv.dyn

static void collect(hybridBVId B, uint64_t len, uint width, uint64_t **bv_D, uint64_t **id_D) {
    //printf("collect\n");
    *bv_D = (uint64_t *) myalloc(((len + w64 - 1) / w64) * sizeof(uint64_t));
    *id_D = (uint64_t *) myalloc(((len * width + w64 - 1) / w64) * sizeof(uint64_t));
    packedRead(B, *id_D, 0);
    myread(B, 0, len, *bv_D, 0);
    hybridBVIdDestroy(B->bv.dyn->left);
    hybridBVIdDestroy(B->bv.dyn->right);
    myfree(B->bv.dyn);
}

// gives number of leaves

static inline uint64_t hybridBVIdLeaves(hybridBVId B) {
    uint num;
    if (B->type == tLeaf) return 1;
    if (B->type == tDynamic) return B->bv.dyn->leaves;
    num = leafBVIdNewSize(B->bv.stat->width);
    return (staticBVIdLength(B->bv.stat) + num - 1) / num;
}

// converts into a leaf if it's short or into a static otherwise
// delta gives the difference in leaves (new - old)

static void flatten(hybridBVId B, int64_t *delta) {
   // printf("flatten\n");
    uint64_t len;
    uint64_t *bv_D, *id_D;
    uint width;

    if (B->type != tDynamic) return;
    width = hybridBVIdWidth(B);
    len = hybridBVIdLength(B);
    *delta = -hybridBVIdLeaves(B);
    collect(B, len, width, &bv_D, &id_D);
    if (len > leafBVIdNewSize(width)) // creates a static
    {
        B->type = tStatic;
        B->bv.stat = staticBVIdCreateFromPacked(bv_D, id_D, len, width);
    } else // a leaf
    {
        B->type = tLeaf;
        B->bv.leaf = leafBVIdCreateFromPacked(bv_D, id_D, 0, len, width, 1);
    }
    *delta += hybridBVIdLeaves(B);
}

// halves a static array into leaves, leaving a leaf covering i
// returns a dynamicId and destroys B

static dynamicBVId splitFrom(uint64_t *data, uint64_t *id_data, uint64_t n, uint64_t ones, uint width, uint64_t i) {
    //printf("splitFrom i=%lu\n", i);
    hybridBVId HB;
    dynamicBVId DB, finalDB;
    uint bnum; // size in elements of blocks to create
    uint bsize; // bit size of blocks to create
    uint64_t *segment, *id_segment;
    uint64_t nblock;
    uint64_t start, mid, end;
    byte *start_ptr, *mid_ptr;

    HB = NULL;
    bnum = leafBVIdNewSize(width); //num elements in block
    bsize = (bnum + 7) / 8; //bytes of a block [bitvector]
    start = 0;
    end = n;
    start_ptr = (byte *) data;
    nblock = (n + bnum - 1) / bnum; // total blocks

    while (nblock >= 2) {
        DB = (dynamicBVId) myalloc(sizeof(struct s_dynamicBVId));
        if (HB == NULL) finalDB = DB;
        else {
            HB->type = tDynamic;
            HB->bv.dyn = DB;
        }
        DB->size = n;
        DB->width = width;
        DB->ones = ones;
        DB->leaves = nblock;
        DB->accesses = 0;
        if (width == w64) DB->last = id_data[end - 1];
        else {
            uint iq = ((end - 1) * width) / w64;
            uint ir = ((end - 1) * width) % w64;
            uint64_t word = (id_data[iq] >> ir);
            if (ir + width > w64) word |= id_data[iq + 1] << (w64 - ir);
            DB->last = word & ((((uint64_t) 1) << width) - 1);
            //printf("DB->last= %lu\n", DB->last);
        }
        mid = start + (nblock / 2) * bnum;
        mid_ptr = start_ptr + (nblock / 2) * bsize;
        if (i / bnum < nblock / 2) {
            // split the left half
            // create right half
            DB->right = HB = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
            if (n - (nblock / 2) * bnum > leafBVIdNewSize(width)) {
                // create a static
                segment = (uint64_t *) myalloc((end - mid + w64 - 1) / w64 * sizeof(uint64_t));
                memcpy(segment, mid_ptr, (end - mid + w64 - 1) / w64 * sizeof(uint64_t));

                id_segment = (uint64_t *) myalloc((((end - mid) * width + w64 - 1) / w64) * sizeof(uint64_t));
                copyBits(id_segment, 0, id_data, mid * width, (end - mid) * width);
                HB->type = tStatic;
                HB->bv.stat = staticBVIdCreateFromPacked(segment, id_segment, n - (nblock / 2) * bnum, width);
            } else {
                // create a leaf
                HB->type = tLeaf;
                HB->bv.leaf = leafBVIdCreateFromPacked((uint64_t *) mid_ptr, id_data, mid, n - (nblock / 2) * bnum,
                                                       width, 0);
            }
            // continue for left half
            end = mid;
            nblock = nblock / 2;
            n = nblock * bnum;
            ones -= hybridBVIdOnes(HB);
            DB->left = HB = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
        } else {
            // split the right half
            // create left half
            DB->left = HB = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
            if ((nblock / 2) * bnum > leafBVIdNewSize(width)) {
                // create a static
                segment = (uint64_t *) myalloc((mid - start + w64 - 1) / w64 * sizeof(uint64_t));
                memcpy(segment, start_ptr, (mid - start + w64 - 1) / w64 * sizeof(uint64_t));
                id_segment = (uint64_t *) myalloc((((mid - start) * width + w64 - 1) / w64)
                                                  * sizeof(uint64_t));
                copyBits(id_segment, 0, id_data, start * width, (mid - start) * width);
                HB->type = tStatic;
                HB->bv.stat = staticBVIdCreateFromPacked(segment, id_segment, (nblock / 2) * bnum, width);
            } else {
                // create a leaf
                HB->type = tLeaf;
                HB->bv.leaf = leafBVIdCreateFromPacked((uint64_t *) start_ptr, id_data, start, (nblock / 2) * bnum,
                                                       width, 0);
            }
            // continue for right half
            start = mid;
            start_ptr = mid_ptr;
            n = n - (nblock / 2) * bnum;
            i = i - (nblock / 2) * bnum;
            ones -= hybridBVIdOnes(HB);
            nblock = nblock - nblock / 2;
            DB->right = HB = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
        }
    }
    // finally, the leaf where i lies
    HB->type = tLeaf;
    HB->bv.leaf = leafBVIdCreateFromPacked((uint64_t *) start_ptr, id_data, start, n, width, 0);
    return finalDB;
}

static dynamicBVId split(staticBVId B, uint64_t i) {
    dynamicBVId DB;
    DB = splitFrom(B->data, B->id_data, staticBVIdLength(B), staticBVIdOnes(B), B->width, i);
    staticBVIdDestroy(B);
    return DB;
}

// balance by rebuilding: flattening + splitting
// assumes B is dynamic

static inline int canBalance(uint64_t n, uint width, int dleft, int dright) {
    uint b = leafBVIdNewSize(width); // size of leaves to create in split
    uint64_t left = (((n + b - 1) / b) / 2) * b; // size of left part
    uint64_t right = n - left; // size of right part
    if (left + dleft > AlphaFactor * (n + dleft + dright)) return 0;
    if (right + dright > AlphaFactor * (n + dleft + dright)) return 0;
    return 1;
}

static void balance(hybridBVId B, uint64_t i, int64_t *delta) {
    //printf("BALANCE\n");
    uint64_t len = hybridBVIdLength(B);
    uint width = hybridBVIdWidth(B);
    uint64_t ones = hybridBVIdOnes(B);
    uint64_t *bv_D, *id_D;
    *delta = -hybridBVIdLeaves(B);
    collect(B, len, width, &bv_D, &id_D);
    B->bv.dyn = splitFrom(bv_D, id_D, len, ones, width, i);
    *delta += hybridBVIdLeaves(B);
    myfree(bv_D);
    myfree(id_D);
}

// splits a full leaf into two
// returns a hybridId and destroys B

static dynamicBVId splitLeaf(leafBVId B) {
    //printf("splitLeaf\n");
    hybridBVId HB, HB1, HB2;
    dynamicBVId DB;
    leafBVId LB;
    uint bnum;

    bnum = leafBVIdMaxSize(B->width) / 2; // elements in new leaves
    uint bsize = (bnum + 7) / 8; // byte size of new left leaf
    LB = leafBVIdCreateFromPacked(B->data, B->id_data, 0, bnum, B->width, 0);
    HB1 = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
    HB1->type = tLeaf;
    HB1->bv.leaf = LB;
    LB = leafBVIdCreateFromPacked((uint64_t *) (((byte *) B->data) + bsize),
                                  B->id_data, bnum, B->size - bnum, B->width, 0);
    HB2 = (hybridBVId) myalloc(sizeof(struct s_hybridBVId));
    HB2->type = tLeaf;
    HB2->bv.leaf = LB;
    DB = (dynamicBVId) myalloc(sizeof(struct s_dynamicBVId));
    DB->size = B->size;
    DB->width = B->width;
    DB->ones = B->ones;
    DB->accesses = 0;
    DB->leaves = 2;
    DB->left = HB1;
    DB->right = HB2;
    DB->last = hybridBVIdAccessId(HB2, bnum - 1);
    leafBVIdDestroy(B);
    return DB;
}

// merge the two leaf children of B into a leaf
// returns a leafId and destroys B

static leafBVId mergeLeaves(dynamicBVId B) {
    leafBVId LB1, LB2;
    //printf("merge leaves\n");
    LB1 = B->left->bv.leaf;
    LB2 = B->right->bv.leaf;
    copyBits(LB1->data, LB1->size, LB2->data, 0, LB2->size);
    copyBits(LB1->id_data, LB1->size * B->width, LB2->id_data, 0, LB2->size * B->width);
    LB1->size += LB2->size;
    LB1->ones += LB2->ones;
    leafBVIdDestroy(LB2);
    myfree(B);
    return LB1;
}

// transfers elements from the right to the left leaves children of B
// tells if it transferred something

static int transferLeft(dynamicBVId B) {
    //printf("trL\n");
    leafBVId LB1, LB2;
    uint i, trf, ones, words;
    uint64_t *segment;
    uint64_t lsize;
    LB1 = B->left->bv.leaf;
    LB2 = B->right->bv.leaf;
    trf = (LB2->size - LB1->size + 1) / 2;
    lsize = LB1->size;

    if (trf < leafBVIdMaxSize(B->width) * TrfFactor) return 0;
    //bitvector
    copyBits(LB1->data, LB1->size, LB2->data, 0, trf);
    LB1->size += trf;
    LB2->size -= trf;
    segment = (uint64_t *) myalloc(bitmapSize(B->width));
    copyBits(segment, 0, LB2->data, trf, LB2->size);
    words = trf / w64;
    ones = 0;
    for (i = 0; i < words; i++) ones += popcount(LB2->data[i]);
    if (trf % w64)
        ones += popcount(LB2->data[words] &
                         (((uint64_t) 1) << (trf % w64)) - 1);
    LB1->ones += ones;
    LB2->ones -= ones;
    memcpy(LB2->data, segment, (LB2->size + 7) / 8);
    myfree(segment);

    //sequence
    copyBits(LB1->id_data, lsize * B->width, LB2->id_data, 0, trf * B->width);
    segment = (uint64_t *) myalloc(sequenceSize(B->width));
    copyBits(segment, 0, LB2->id_data, trf * B->width, LB2->size * B->width);
    memcpy(LB2->id_data, segment, (LB2->size * B->width + 7) / 8);
    myfree(segment);
    return 1;
}

// transfers elems from the left to the right leaves children of B
// tells if it transferred something

static int transferRight(dynamicBVId B) {
    //printf("trR\n");
    leafBVId LB1, LB2;
    uint i, trf, ones, words;
    uint64_t *segment, *id_segment;

    LB1 = B->left->bv.leaf;
    LB2 = B->right->bv.leaf;
    trf = (LB1->size - LB2->size + 1) / 2;
    if (trf < leafBVIdMaxSize(B->width) * TrfFactor) return 0;
    //bitvector
    segment = (uint64_t *) myalloc(leafMaxSize() * sizeof(uint64_t));
    memcpy(segment, LB2->data, (LB2->size + 7) / 8);
    copyBits(LB2->data, 0, LB1->data, LB1->size - trf, trf);
    words = trf / w64;
    ones = 0;
    for (i = 0; i < words; i++) ones += popcount(LB2->data[i]);
    if (trf % w64)
        ones += popcount(LB2->data[words] &
                         (((uint64_t) 1) << (trf % w64)) - 1);
    LB1->ones -= ones;
    LB2->ones += ones;
    copyBits(LB2->data, trf, segment, 0, LB2->size);
    myfree(segment);

    //sequence
    segment = (uint64_t *) myalloc(sequenceSize(B->width));
    memcpy(segment, LB2->id_data, (LB2->size * B->width + 7) / 8);
    copyBits(LB2->id_data, 0, LB1->id_data, (LB1->size - trf) * B->width, trf * B->width);
    copyBits(LB2->id_data, trf * B->width, segment, 0, LB2->size * B->width);
    myfree(segment);

    LB1->size -= trf;
    LB2->size += trf;
    return 1;
}

// writes B to file, which must be opened for writing

void hybridBVIdSave(hybridBVId B, FILE *file) {
    int64_t delta;
    flatten(B, &delta);
    myfwrite(&B->type, sizeof(nodeType), 1, file);
    if (B->type == tLeaf) leafBVIdSave(B->bv.leaf, file);
    else staticBVIdSave(B->bv.stat, file);
}

// loads hybridId from file, which must be opened for reading

hybridBVId hybridBVIdLoad(FILE *file) {
    uint64_t size;
    hybridBVId B = myalloc(sizeof(struct s_hybridBVId));
    nodeType type;
    myfread(&type, sizeof(nodeType), 1, file);
    B->type = type;
    if (B->type == tStatic) {
        B->bv.stat = staticBVIdLoad(file);
    } else {
        B->bv.leaf = leafBVIdLoad(file);
    }
    return B;
}

// gives space of hybridId in w-bit words

uint64_t hybridBVIdSpace(hybridBVId B) {
    uint64_t s;
    s = (sizeof(struct s_hybridBVId) * 8 + w64 - 1) / w64;
    if (B->type == tLeaf) return s + leafBVIdSpace(B->bv.leaf);
    else if (B->type == tStatic) return s + staticBVIdSpace(B->bv.stat);
    return s + (sizeof(struct s_dynamicBVId) * 8 + w64 - 1) / w64 +
           hybridBVIdSpace(B->bv.dyn->left) + hybridBVIdSpace(B->bv.dyn->right);
}


// gives number of elements

extern inline uint64_t hybridBVIdLength(hybridBVId B) {
    if (B->type == tLeaf) return leafBVIdLength(B->bv.leaf);
    if (B->type == tStatic) return staticBVIdLength(B->bv.stat);
    return B->bv.dyn->size;
}

extern inline uint64_t hybridBVIdLast(hybridBVId B) {
    if (B->type == tLeaf) return leafBVIdAccessId(B->bv.leaf, leafBVIdLength(B->bv.leaf)-1);
    if (B->type == tStatic) return staticBVIdAccessId(B->bv.stat, staticBVIdLength(B->bv.stat)-1);
    return B->bv.dyn->last;
}

// gives width of elements

extern inline uint hybridBVIdWidth(hybridBVId B) {
    if (B->type == tStatic) return B->bv.stat->width;
    if (B->type == tLeaf) return B->bv.leaf->width;
    return B->bv.dyn->width;
}

extern inline uint64_t hybridBVIdOnes(hybridBVId B) {
    if (B->type == tLeaf) return leafBVIdOnes(B->bv.leaf);
    if (B->type == tStatic) return staticBVIdOnes(B->bv.stat);
    return B->bv.dyn->ones;
}

// checks balance of B, assumed to be dynamic, and rotates once if needs

// sets value for B[i] = v, assumes i is right and v fits in width

int hybridBVIdWriteBV(hybridBVId B, uint64_t i, uint64_t v) {
    uint64_t lsize;
    int dif;
    if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat, i);
    }
    if (B->type == tLeaf) {
        dif = leafBVIdWriteBV(B->bv.leaf, i, v);
        return dif;
    }
    B->bv.dyn->accesses = 0; // reset
    lsize = hybridBVIdLength(B->bv.dyn->left);
    if (i < lsize) dif = hybridBVIdWriteBV(B->bv.dyn->left, i, v);
    else dif = hybridBVIdWriteBV(B->bv.dyn->right, i - lsize, v);
    B->bv.dyn->ones += dif;
    return dif;
}

void hybridBVIdSplit(hybridBVId B, uint64_t i) {
    uint64_t lsize;
    if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat, i);
        return;
    }
    if (B->type == tLeaf) {
        if (leafBVIdLength(B->bv.leaf) == leafBVIdMaxSize(B->bv.leaf->width)) //split
        {
            B->type = tDynamic;
            B->bv.dyn = splitLeaf(B->bv.leaf);
        }
        return;
    }
    lsize = hybridBVIdLength(B->bv.dyn->left);
    if (i < lsize) hybridBVIdSplit(B->bv.dyn->left, i);
    else hybridBVIdSplit(B->bv.dyn->right, i - lsize);
}

void hybridBVIdSplitMax(hybridBVId B) {
    uint64_t length = hybridBVIdLength(B);
    uint64_t b = leafBVIdNewSize(hybridBVIdWidth(B));
    for(uint64_t i = b-1; i < length; i += b) {
        hybridBVIdSplit(B, i);
    }
}

// changing leaves is uncommon and only then we need to recompute
// leaves. we do our best to avoid this overhead in typical operations

static void irecompute(hybridBVId B, uint64_t i) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        lsize = hybridBVIdLength(B->bv.dyn->left);
        if (i < lsize) irecompute(B->bv.dyn->left, i);
        else irecompute(B->bv.dyn->right, i - lsize);
        B->bv.dyn->leaves = hybridBVIdLeaves(B->bv.dyn->left) +
                            hybridBVIdLeaves(B->bv.dyn->right);
    }
}

static void rrecompute(hybridBVId B, uint64_t i, uint64_t l) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        lsize = hybridBVIdLength(B->bv.dyn->left);
        if (i + l < lsize) rrecompute(B->bv.dyn->left, i, l);
        else if (i >= lsize) rrecompute(B->bv.dyn->right, i - lsize, l);
        else {
            rrecompute(B->bv.dyn->left, i, lsize - i);
            rrecompute(B->bv.dyn->right, 0, l - (lsize - i));
        }
        B->bv.dyn->leaves = hybridBVIdLeaves(B->bv.dyn->left) +
                            hybridBVIdLeaves(B->bv.dyn->right);
    }
}

// inserts v at B[i], assumes i is right and v fits in width
// returns last value in subtree, -1 if it did not change
static int64_t insert(hybridBVId B, uint64_t i, uint bv_v, uint64_t id_v, uint first, uint *recalc) {
    uint64_t lsize, rsize;
    int64_t delta, last;
    uint width;
    if (B->type == tStatic) {
        B->type = tDynamic;
        B->bv.dyn = split(B->bv.stat, i); // does not change #leaves!
    }
    if (B->type == tLeaf) {
        if (leafBVIdLength(B->bv.leaf) == leafBVIdMaxSize(B->bv.leaf->width)) //split
        {
            B->type = tDynamic;
            B->bv.dyn = splitLeaf(B->bv.leaf);
            *recalc = 1; // leaf added
        } else {
            leafBVIdInsert(B->bv.leaf, i, bv_v, id_v, first);
            return leafBVIdAccessId(B->bv.leaf, leafBVIdLength(B->bv.leaf) - 1);
        }
    }
    B->bv.dyn->accesses = 0; // reset
    width = B->bv.dyn->width;
    lsize = hybridBVIdLength(B->bv.dyn->left);
    rsize = hybridBVIdLength(B->bv.dyn->right);
    if (i < lsize) {
        if ((lsize == leafBVIdMaxSize(width)) // will overflow if leaf
            && (rsize < leafBVIdMaxSize(width)) // can avoid if leaf
            && (B->bv.dyn->left->type == tLeaf) // both are leaves
            && (B->bv.dyn->right->type == tLeaf)
            && transferRight(B->bv.dyn)) {
            // avoided, transferred to the right
            return insert(B, i, bv_v, id_v, first, recalc);
        }
        if ((lsize + 1 > AlphaFactor * (lsize + rsize + 1))
            && (lsize + rsize >= MinLeavesToBalance * leafBVIdMaxSize(width))
            && canBalance(lsize + rsize, width, 1, 0)) {
            // too biased
            delta = 0;
            balance(B, i, &delta);
            if (delta) *recalc = 1;
            return insert(B, i, bv_v, id_v, first, recalc);
        }
        insert(B->bv.dyn->left, i, bv_v, id_v, first, recalc);
        last = -1; // cannot alter previous last value
    } else {
        if ((rsize == leafBVIdMaxSize(width)) // will overflow if leaf
            && (lsize < leafBVIdMaxSize(width)) // can avoid if leaf
            && (B->bv.dyn->right->type == tLeaf) // both are leaves
            && (B->bv.dyn->left->type == tLeaf)
            && transferLeft(B->bv.dyn)) {
            // avoided, transferred to the left
            return insert(B, i, bv_v, id_v, first, recalc);
        }
        if ((rsize + 1 > AlphaFactor * (lsize + rsize + 1))
            && (lsize + rsize >= MinLeavesToBalance * leafBVIdMaxSize(width))
            && canBalance(lsize + rsize, width, 0, 1)) {
            // too biased
            delta = 0;
            balance(B, i, &delta);
            if (delta) *recalc = 1;
            return insert(B, i, bv_v, id_v, first, recalc);
        }
        last = insert(B->bv.dyn->right, i - lsize, bv_v, id_v, first, recalc);
    }
    B->bv.dyn->size++;
    B->bv.dyn->ones += bv_v;
    if (last != -1) B->bv.dyn->last = last;
    return last;
}

void hybridBVIdInsert(hybridBVId B, uint64_t i, uint bv_v, uint64_t id_v, uint first) {
    uint recalc = 0;
    insert(B, i, bv_v, id_v, first, &recalc);
    if (recalc) irecompute(B, i); // we went to the leaf now holding i
}

typedef struct{
    int diff;
    int64_t last;
} res_delete;
// deletes B[i], assumes i is right
// last from deleted, -1 if has not changed
static res_delete delete(hybridBVId B, uint64_t i, uint more, uint *recalc, uint *nl) {

    uint64_t lsize, rsize;
    hybridBVId B2;
    int64_t delta;
    res_delete r;
    uint width;
    if (B->type == tStatic) {
        B->type = tDynamic;
        //printf("split on delete\n"); fflush(stdout);
        B->bv.dyn = split(B->bv.stat, i);
    }
    if (B->type == tLeaf) {
        r.diff = leafBVIdDelete(B->bv.leaf, i, more, nl);
        if(leafBVIdLength(B->bv.leaf) > 0) {
            r.last = leafBVIdAccessId(B->bv.leaf, leafBVIdLength(B->bv.leaf) - 1);
            //printf("last: %lu\n", r.last);
        }
        return r;
    }
    B->bv.dyn->accesses = 0; // reset
    width = B->bv.dyn->width;
    lsize = hybridBVIdLength(B->bv.dyn->left);
    rsize = hybridBVIdLength(B->bv.dyn->right);
    //printf("i=%lu lsize=%lu, rsize=%lu\n", i, lsize, rsize); fflush(stdout);
    if (i < lsize) {
        if ((rsize > AlphaFactor * (lsize + rsize - 1))
            && (lsize + rsize >= MinLeavesToBalance * leafBVIdMaxSize(width))
            && canBalance(lsize + rsize, width, -1, 0)) {
            // too biased
            delta = 0;
            balance(B, i, &delta);
            //printf("balance 1\n");
            if (delta) *recalc = 1;
            return delete(B, i, more, recalc, nl);
        }
        //printf("delete 1\n");
        r = delete(B->bv.dyn->left, i, more, recalc, nl);
        r.last = -1;
        //printf("delete 1 last=%lu\n", r.last);
        if (lsize == 1) {
            // left child is now of size zero, remove
            hybridBVIdDestroy(B->bv.dyn->left);
            B2 = B->bv.dyn->right;
            myfree(B->bv.dyn);
            *B = *B2;
            *recalc = 1;
            //printf("lsize=1 last=%lu\n", r.last);
            return r;
        }
    } else {
        if ((lsize > AlphaFactor * (lsize + rsize - 1))
            && (lsize + rsize >= MinLeavesToBalance * leafBVIdMaxSize(width))
            && canBalance(lsize + rsize, width, 0, -1)) {
            // too biased
            delta = 0;
            balance(B, i, &delta);
            //printf("balance 2\n");
            if (delta) *recalc = 1;
            return delete(B, i, more, recalc, nl);
        }
        //printf("delete 2\n");
        r = delete(B->bv.dyn->right, i - lsize, more, recalc, nl);
        //printf("delete 2 last=%lu\n", r.last);
        if (rsize == 1) {
            // right child is now of size zero, remove
            hybridBVIdDestroy(B->bv.dyn->right);
            B2 = B->bv.dyn->left;
            myfree(B->bv.dyn);
            *B = *B2;
            *recalc = 1;
            //printf("rsize=1 last=%lu\n", B->bv.dyn->last);
            r.last = hybridBVIdLast(B);
            return r;
        }
    }
    B->bv.dyn->size--;
    B->bv.dyn->ones += r.diff;
    if (r.last != -1) {
        B->bv.dyn->last = r.last;
        // printf("end last=%lu\n", r.last);
    }
    if (B->bv.dyn->size <= leafBVIdNewSize(B->bv.dyn->width)) {
        // merge leaves
        //printf("merge leaves\n");
        B->bv.leaf = mergeLeaves(B->bv.dyn);
        B->type = tLeaf;
        *recalc = 1;
        return r;
    }
    if (B->bv.dyn->size <
        B->bv.dyn->leaves * leafBVIdNewSize(B->bv.dyn->width) * MinFillFactor) {
        //printf("flatten\n");
        delta = 0;
        flatten(B, &delta);
        if (delta) *recalc = 1;
        return r;
    }
   // printf("return last=%lu\n", r.last);
    return r;
}

int hybridBVIdDelete(hybridBVId B, uint64_t i, uint more) {
    uint recalc = 0, nl = 0;
    res_delete r = delete(B, i, more, &recalc, &nl);
    if (recalc) {
        //printf("recalc\n");
        // the node is now at i-1 or at i, hard to know
        irecompute(B, i - 1);
        irecompute(B, i);
    }
    if (nl) hybridBVIdWriteBV(B, i, 1); //restoring a 1-bit
    return r.diff;
}

// flattening is uncommon and only then we need to recompute
// leaves. we do our best to avoid this overhead in typical queries

static void recompute(hybridBVId B, uint64_t i, int64_t delta) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        B->bv.dyn->leaves += delta;
        lsize = hybridBVIdLength(B->bv.dyn->left);
        if (i < lsize) recompute(B->bv.dyn->left, i, delta);
        else recompute(B->bv.dyn->right, i - lsize, delta);
    }
}

// access B[i], assumes i is right

uint access(hybridBVId B, uint64_t i, int64_t *delta, uint64_t *id) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        B->bv.dyn->accesses++;
        if (mustFlatten(B))
            flatten(B, delta);
        else {
            lsize = hybridBVIdLength(B->bv.dyn->left);
            if (i < lsize) {
                return access(B->bv.dyn->left, i, delta, id);
            } else {
                return access(B->bv.dyn->right, i - lsize, delta, id);
            }
        }
    }
    if (B->type == tLeaf) {
        *id = leafBVIdAccessId(B->bv.leaf, i);
        return leafBVIdAccessBV(B->bv.leaf, i);
    } else {
        *id = staticBVIdAccessId(B->bv.stat, i);
        return staticBVIdAccessBV(B->bv.stat, i);
    }
}

uint64_t accessId(hybridBVId B, uint64_t i, int64_t *delta) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        B->bv.dyn->accesses++;
        if (mustFlatten(B)){
            flatten(B, delta);
        }else {
            lsize = hybridBVIdLength(B->bv.dyn->left);
            if (i < lsize) {
                return accessId(B->bv.dyn->left, i, delta);
            } else {
                return accessId(B->bv.dyn->right, i - lsize, delta);
            }
        }
    }
    if (B->type == tLeaf) {
        return leafBVIdAccessId(B->bv.leaf, i);
    } else {
        return staticBVIdAccessId(B->bv.stat, i);
    }
}

uint64_t hybridBVIdAccess(hybridBVId B, uint64_t i, uint64_t *id) {
    int64_t delta = 0;
    uint64_t answ = access(B, i, &delta, id);
    if (delta) recompute(B, i, delta);
    return answ;
}

uint64_t hybridBVIdAccessId(hybridBVId B, uint64_t i) {
    int64_t delta = 0;
    uint64_t answ = accessId(B, i, &delta);
    if (delta) recompute(B, i, delta);
    return answ;
}


static uint64_t rank(hybridBVId B, uint64_t i, int64_t *delta) {
    uint64_t lsize;
    if (B->type == tDynamic) {
        B->bv.dyn->accesses++;
        if (mustFlatten(B))
            flatten(B, delta);
        else {
            lsize = hybridBVIdLength(B->bv.dyn->left);
            if (i < lsize) return rank(B->bv.dyn->left, i, delta);
            else
                return hybridBVIdOnes(B->bv.dyn->left) +
                       rank(B->bv.dyn->right, i - lsize, delta);
        }
    }
    if (B->type == tLeaf) return leafBVIdRank(B->bv.leaf, i);
    return staticBVIdRank(B->bv.stat, i);
}


uint64_t hybridBVIdRank(hybridBVId B, uint64_t i) {
    int64_t delta = 0;
    uint64_t answ = rank(B, i, &delta);
    if (delta) recompute(B, i, delta);
    return answ;
}

// computes select_1(B,j), zero-based, assumes j is right

static uint64_t select1(hybridBVId B, uint64_t j, int64_t *delta) {
    uint64_t lones;
    if (B->type == tDynamic) {
        B->bv.dyn->accesses++;
        if (mustFlatten(B))
            flatten(B, delta);
        else {
            lones = hybridBVIdOnes(B->bv.dyn->left);
            if (j <= lones) return select1(B->bv.dyn->left, j, delta);
            return hybridBVIdLength(B->bv.dyn->left)
                   + select1(B->bv.dyn->right, j - lones, delta);
        }
    }
    if (B->type == tLeaf) return leafBVIdSelect(B->bv.leaf, j);
    return staticBVIdSelect(B->bv.stat, j);
}

uint64_t hybridBVIdSelect(hybridBVId B, uint64_t j) {
    int64_t delta = 0;
    uint64_t answ = select1(B, j, &delta);
    if (delta) recompute(B, answ, delta);
    return answ;
}

static int64_t next1(hybridBVId B, uint64_t i, int64_t *delta) {
    uint64_t lsize;
    int64_t next;
    if (hybridBVIdOnes(B) == 0) return -1;
    if (B->type == tDynamic) {
        B->bv.dyn->accesses++; // not considered an access!
        if (mustFlatten(B))
            flatten(B, delta);
        else {
            lsize = hybridBVIdLength(B->bv.dyn->left);
            if (i < lsize) {
                next = next1(B->bv.dyn->left, i, delta);
                if (next != -1) return next;
                i = lsize;
            }
            //return lsize + next1(B->bv.dyn->right,i-lsize,delta);
            next = next1(B->bv.dyn->right, i - lsize, delta);
            if (next != -1) return lsize + next;
            return -1;
        }
    }
    if (B->type == tLeaf) return leafBVIdNext1(B->bv.leaf, i);
    return staticBVIdNext1(B->bv.stat, i);
}

int64_t hybridBVIdNext1(hybridBVId B, uint64_t i) {
    int64_t delta = 0;
    int64_t answ = next1(B, i, &delta);
    if (delta) recompute(B, i, delta);
    return answ;
}

// finds first value >= c in [i..j], which must be increasing
// returns j+1 if not found

static uint64_t next(hybridBVId B, uint64_t i, uint64_t j, uint64_t c,
                     uint *recomp, uint64_t *value, uint all) {
    uint64_t lsize;
    int64_t delta;
    int64_t n;
    if (B->type == tDynamic) {
        if (all && B->bv.dyn->last < c) return j + 1;
        B->bv.dyn->accesses++;
        if (mustFlatten(B)) {
            delta = 0;
            flatten(B, &delta);
            if (delta) *recomp = 1;
        } else {
            lsize = hybridBVIdLength(B->bv.dyn->left);
            if (j < lsize)
                return next(B->bv.dyn->left, i, j, c, recomp, value, j == lsize - 1);
            if (i >= lsize)
                return lsize + next(B->bv.dyn->right, i - lsize, j - lsize, c, recomp, value, all);
            n = next(B->bv.dyn->left, i, lsize - 1, c, recomp, value, 1);
            if (n < lsize) return n;
            return lsize + next(B->bv.dyn->right, 0, j - lsize, c, recomp, value, all);
        }
    }
    if (B->type == tLeaf) return leafBVIdNext(B->bv.leaf, i, j, c, value);
    else return staticBVIdNext(B->bv.stat, i, j, c, value);
}


uint64_t hybridBVIdNext(hybridBVId B, uint64_t i, uint64_t j, uint64_t c, uint64_t *value) {
    uint recomp = 0;
    uint64_t n = next(B, i, j, c, &recomp, value, 0);
    if (recomp) rrecompute(B, i, j - i + 1);
    return n;
}

uint checkOnes(hybridBVId B) {
    if (B->type == tDynamic) {
        uint64_t ones = hybridBVIdOnes(B->bv.dyn->left) + hybridBVIdOnes(B->bv.dyn->right);
        return ones == hybridBVIdOnes(B) && checkOnes(B->bv.dyn->left) && checkOnes(B->bv.dyn->right);
    }
    if(B->type == tLeaf) {
        return leafBVIdCheckOnes(B->bv.leaf);
    }
    if(B->type == tStatic) {
        return staticBVIdCheckOnes(B->bv.stat);
    }
    return 1;
}

uint checkLeaves(hybridBVId B) {
    if (B->type == tDynamic) {
        return checkLeaves(B->bv.dyn->left) && checkLeaves(B->bv.dyn->right);
    }
    if(B->type == tLeaf) {
        return 1;
    }
    return 0;
}


uint checkLast(hybridBVId B) {
    if (B->type == tDynamic) {
        return checkLast(B->bv.dyn->left) && (hybridBVIdLast(B->bv.dyn->right) == hybridBVIdLast(B));
    }
    return 1;
}

uint checkLastPrint(hybridBVId B) {
    if (B->type == tDynamic) {
        checkLastPrint(B->bv.dyn->left);
        if(hybridBVIdLast(B->bv.dyn->right) != hybridBVIdLast(B)){
            printf("falla en dynamic con: %lu, %lu\n", hybridBVIdLast(B), hybridBVIdLength(B));
            if (B->bv.dyn->left->type == tStatic) {
                printf("-left: static %lu, %lu \n", hybridBVIdLast(B->bv.dyn->left), hybridBVIdLength(B->bv.dyn->left));
            } else if (B->bv.dyn->left->type == tLeaf) {
                printf("-left: leaf %lu, %lu \n", hybridBVIdLast(B->bv.dyn->left), hybridBVIdLength(B->bv.dyn->left));
            } else {
                printf("-left: dynamic %lu, %lu \n", hybridBVIdLast(B->bv.dyn->left), hybridBVIdLength(B->bv.dyn->left));
            }
            if (B->bv.dyn->right->type == tStatic) {
                printf("-right: static %lu, %lu \n", hybridBVIdLast(B->bv.dyn->right), hybridBVIdLength(B->bv.dyn->right));
            } else if (B->bv.dyn->right->type == tLeaf) {
                printf("-right: leaf %lu, %lu \n", hybridBVIdLast(B->bv.dyn->right), hybridBVIdLength(B->bv.dyn->right));
            } else {
                printf("-right: dynamic %lu, %lu \n", hybridBVIdLast(B->bv.dyn->right), hybridBVIdLength(B->bv.dyn->right));
            }
            printf("\n");
            fflush(stdout);
            return 0;
        }
    }
    return 1;
}


uint checkOnesPrint(hybridBVId B) {
    if (B->type == tDynamic) {
        uint64_t ones = hybridBVIdOnes(B->bv.dyn->left) + hybridBVIdOnes(B->bv.dyn->right);
        if (ones == hybridBVIdOnes(B)) {
            return checkOnesPrint(B->bv.dyn->left) && checkOnesPrint(B->bv.dyn->right);
        }
        printf("falla en dynamic con: %lu\n", hybridBVIdOnes(B));
        if (B->bv.dyn->left->type == tStatic) {
            printf("-left: static %lu \n", hybridBVIdOnes(B->bv.dyn->left));
        } else if (B->bv.dyn->left->type == tLeaf) {
            printf("-left: leaf %lu \n", hybridBVIdOnes(B->bv.dyn->left));
        } else {
            printf("-left: dynamic %lu \n", hybridBVIdOnes(B->bv.dyn->left));
        }

        if (B->bv.dyn->right->type == tStatic) {
            printf("-right: static %lu \n", hybridBVIdOnes(B->bv.dyn->right));
        } else if (B->bv.dyn->right->type == tLeaf) {
            printf("-right: leaf %lu \n", hybridBVIdOnes(B->bv.dyn->right));
        } else {
            printf("-right: dynamic %lu \n", hybridBVIdOnes(B->bv.dyn->right));
        }
        printf("\n");
        fflush(stdout);
        return 0;
    }
    return 1;
}
