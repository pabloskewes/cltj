
#include "hybridBV/staticBVId.h"

#define K 4 // block length is w*K

#define w16 (8*sizeof(uint16_t)) // superblock length is 2^w16

	// preprocesses for rank, with parameter K

static void staticPreprocess (staticBVId B)

    { uint64_t i,n;
      uint64_t sacc,acc;
      n = B->size;
      if (n == 0) return;
      B->B = (uint16_t*)myalloc(((n+K*w64-1)/(K*w64))*sizeof(uint16_t));
      B->S = (uint64_t*)myalloc(((n+(1<<w16)-1)/(1<<w16))*sizeof(uint64_t));
      sacc = acc = 0;
      i = 0;
      while (i<(n+w64-1)/w64)
          { uint64_t top = mymin((n+w64-1)/w64,i+(1<<w16)/w64);
            sacc += acc; acc = 0;
            B->S[(i*w64) >> w16] = sacc;
            while (i<top)
                { if (i%K == 0) B->B[i/K] = acc;
                  acc += popcount(B->data[i]);
                  i++;
                }
          }
      B->ones = staticBVIdRank(B,n-1);
    }

	// converts a bit array into a bitvector of n bits
        // data is pointed to and will be freed 

staticBVId staticBVIdCreateFrom64(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width)

    { staticBVId B;
	  uint p, i;
	  uint64_t word;
      B = (staticBVId)myalloc(sizeof(struct s_staticBVId));
      B->size = n;
      B->width = width;
      if (n == 0) {
      	B->data = NULL;
      	B->id_data = NULL;
      } else {
      	B->data = bv_data;
      	B->id_data = (uint64_t *) mycalloc((n*width+w64-1)/w64, sizeof(uint64_t));
      	p = 0;
      	for (i=0;i<n;i++)
      	{   word = id_data[i];
      		B->id_data[p/w64] |= word << (p%w64);
      		if ((p%w64)+width > w64) B->id_data[p/w64+1] = word >> (w64-(p%w64));
      		p += width;
      	}
      }
      B->S = NULL;
      B->B = NULL;
      staticPreprocess(B);
      return B;
    }

staticBVId staticBVIdCreateFromPacked(uint64_t *bv_data, uint64_t *id_data, uint64_t n, uint width)

{ staticBVId B;
	uint p, i;
	uint64_t word;
	B = (staticBVId)myalloc(sizeof(struct s_staticBVId));
	B->size = n;
	B->width = width;
	if (n == 0) {
		B->data = NULL;
		B->id_data = NULL;
	} else {
		B->data = bv_data;
		B->id_data = id_data;
	}
	B->S = NULL;
	B->B = NULL;
	staticPreprocess(B);
	return B;
}


staticBVId staticBVIdClone(staticBVId B) {
	staticBVId BC = (staticBVId)myalloc(sizeof(struct s_staticBVId));
	BC->size = B->size;
	BC->width = B->width;
	BC->ones = B->ones;
	if(BC->size == 0) {
		BC->data = NULL;
		BC->id_data = NULL;
		BC->S = NULL;
		BC->B = NULL;
	}else {
		BC->data = (uint64_t*)myalloc( ((BC->size+w64-1)/w64)* sizeof(uint64_t));
		BC->id_data = (uint64_t*) myalloc(((BC->size*BC->width+w64-1)/w64)*sizeof(uint64_t));
		BC->B = (uint16_t*)myalloc(((BC->size+K*w64-1)/(K*w64))*sizeof(uint16_t));
		BC->S = (uint64_t*)myalloc(((BC->size+(1<<w16)-1)/(1<<w16))*sizeof(uint64_t));
		memcpy(BC->data,B->data, ((BC->size+w64-1)/w64)* sizeof(uint64_t));
		memcpy(BC->id_data,B->id_data, ((BC->size*BC->width+w64-1)/w64)*sizeof(uint64_t));
		memcpy(BC->B,B->B, ((BC->size+K*w64-1)/(K*w64))*sizeof(uint16_t));
		memcpy(BC->S,B->S, ((BC->size+(1<<w16)-1)/(1<<w16))*sizeof(uint64_t));
	}
	return BC;
}

	// destroys B, frees data

void staticBVIdDestroy (staticBVId B)

    { if (B != NULL) 
         { myfree(B->data);
           myfree(B->S); myfree(B->B);
           myfree(B->id_data);
      	   myfree(B);
	 }
    }

        // writes B's data to file, which must be opened for writing 

void staticBVIdSave (staticBVId B, FILE *file)

{   myfwrite(&B->size, sizeof(uint64_t), 1, file);
	myfwrite(&B->width, sizeof(byte), 1, file);
	myfwrite(&B->ones, sizeof(uint64_t), 1, file);
	if (B->size != 0) {
		myfwrite(B->data, sizeof(uint64_t), (B->size + w64 - 1) / w64, file); //bitvector
		myfwrite(B->id_data, sizeof(uint64_t), (B->size * B->width + w64 - 1) / w64, file); //sequence
	}
   }

        // loads staticBV's data from file, which must be opened for reading
	// size is the number of bits


staticBVId staticBVIdLoad (FILE *file)

    { staticBVId B;
		uint64_t size, ones;
		byte width;
		myfread(&size, sizeof(uint64_t), 1, file);
		myfread(&width, sizeof(byte), 1, file);
		myfread(&ones, sizeof(uint64_t), 1, file);
		B = (staticBVId)myalloc(sizeof(struct s_staticBVId));
      B->size = size;
      B->width = width;
	  B->ones = ones;
      if (size == 0) {
      	B->data = NULL;
      	B->id_data = NULL;
      }
      else {
         B->data = (uint64_t*)myalloc(((B->size+w64-1)/w64)*sizeof(uint64_t));
	     fread (B->data,sizeof(uint64_t),(B->size+w64-1)/w64,file);
      	 B->id_data = (uint64_t *) myalloc(((B->size * B->width + w64 - 1) / w64) * sizeof(uint64_t));
      	 myfread(B->id_data, sizeof(uint64_t), (B->size * B->width + w64 - 1) / w64, file);
	   }
      B->S = NULL;
      B->B = NULL;
      staticPreprocess(B);
      return B;
    }

	// data of staticBV

extern inline uint64_t *staticBVIdBits (staticBVId B)

    { return B->data;
    }

extern inline uint64_t *staticBVIdIds (staticBVId B)

{ return B->id_data;
}

	// staticBV size in w-bit words

uint64_t staticBVIdSpace (staticBVId B)

    { uint64_t space = sizeof(struct s_staticBVId)*8/w64;
      if (B == NULL) return 0;
      if (B->data != NULL) space += (B->size+w64-1)/w64;
      if (B->id_data != NULL) space += (B->size * B->width + w64 - 1) / w64;
      if (B->B != NULL) space += ((B->size+K*w64-1)/(K*w64))/(w64/w16);
      if (B->S != NULL) space += (B->size+(1<<w16)-1)/(1<<w16);
      return space;
    }

        // gives bit length

extern inline uint64_t staticBVIdLength (staticBVId B)

    { return B->size;
    }

        // gives number of ones

extern inline uint64_t staticBVIdOnes (staticBVId B)

    { return B->ones;
    }

	// access B[i], assumes i is right

extern inline uint staticBVIdAccessBV (staticBVId B, uint64_t i)

    { return (B->data[i/w64] >> (i%w64)) & 1;
    }

extern inline uint64_t staticBVIdAccessId(staticBVId B, uint i) {
	uint64_t word;
	if (B->width == w64) return B->id_data[i];
	uint iq = i * B->width / w64;
	uint ir = (i * B->width) % w64;
	word = (B->id_data[iq] >> ir);
	if (ir + B->width > w64) word |= B->id_data[iq + 1] << (w64 - ir);
	return word & ((((uint64_t) 1) << B->width) - 1);
}

        // read bits [i..i+l-1] onto D[j..], assumes it is right
        
extern inline void staticBVIdReadBV (staticBVId B, uint64_t i, uint64_t l,
			       uint64_t * D, uint64_t j)

    { copyBits(D,j,B->data,i,l);
    }

void staticBVIdReadId(staticBVId B, uint i, uint l, uint64_t *D) {
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

extern inline uint64_t staticBVIdRank (staticBVId B, uint64_t i)

    { uint64_t b,sb;
      uint64_t rank;
      sb = i/(K*w64);
      rank = B->S[i>>w16] + B->B[sb];
      sb *= K;
      for (b=sb;b<i/w64;b++) rank += popcount(B->data[b]);
      return rank + popcount(B->data[b] & (((uint64_t)~0) >> (w64-1-(i%w64))));
    }

        // computes select_1(B,j), zero-based, assumes j is right

extern uint64_t staticBVIdSelect (staticBVId B, uint64_t j)

    { int64_t i,d,b;
      uint p;
      uint64_t word,s,m,n;
      n = B->size;
      s = (n+(1<<w16)-1)/(1<<w16);
	// interpolation: guess + exponential search
      i = ((uint64_t)(j * (n / (float)B->ones))) >> w16;
      if (i == s) i--;
      if (B->S[i] < j)
	 { d = 1;
	   while ((i+d < s) && (B->S[i+d] < j))
	      { i += d; d <<= 1; }
	   d = mymin(s,i+d); // now d is the top of the range
	   while (i+1<d)
	      { m = (i+d)>>1;
	        if (B->S[m] < j) i = m; else d = m;
	      }
	 }
      else
	 { d = 1;
	   while ((i-d >= 0) && (B->S[i-d] >= j))
	      { i -= d; d <<= 1; }
	   d = mymax(0,i-d); // now d is the bottom of the range
	   while (d+1<i)
	      { m = (i+d)>>1;
	        if (B->S[m] < j) d = m; else i = m;
	      }
	   i--;
	 }
	// now the same inside the superblock
      j -= B->S[i]; // what remains to be found inside the superblock
      p = i < s-1 ? B->S[i+1]-B->S[i] : B->ones-B->S[i];
      b = (i << w16) / (w64*K);
      s = mymin(b+(1<<w16)/(w64*K),(n+w64*K-1)/(w64*K));
      i = b + ((j * (s-b)*(w64*K) / (float)p)) / (w64*K);
      if (i == s) i--;
      if (B->B[i] < j)
	 { d = 1;
	   while ((i+d < s) && (B->B[i+d] < j))
	      { i += d; d <<= 1; }
	   d = mymin(s,i+d); // now d is the top of the range
	   while (i+1<d)
	      { m = (i+d)>>1;
	        if (B->B[m] < j) i = m; else d = m;
	      }
	 }
      else
	 { d = 1;
	   while ((i-d >= b) && (B->B[i-d] >= j))
	      { i -= d; d <<= 1; }
	   d = mymax(b,i-d); // now d is the bottom of the range
	   while (d+1<i)
	      { m = (i+d)>>1;
	        if (B->B[m] < j) d = m; else i = m;
	      }
	   i--;
	 }
	// now it's confined to K blocks
      j -= B->B[i];
      i *= K;
      while ((i+1)*w64 < n)
	{ p = popcount(B->data[i]);
	  if (p >= j) break;
	  j -= p;
	  i++;
	}
      word = B->data[i];
	  //printf("word: %lu\n", word);
      i *= w64;
      while (1)
	{ j -= word & 1;
	  if (j == 0) return i;
	  word >>= 1;
	  i++;
	}
    }
