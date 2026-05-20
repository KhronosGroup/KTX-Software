/* extracted/copied from: bcdec.h - v0.97
   written by Sergii "iOrange" Kudlai in 2022

   provides functions to decompress blocks of BC6H compressed images

   BC6H is expected to decompress into 4*4 RGB blocks of either 32bit float or
   16bit "half" per component (96bit or 48bit pixel)

   LICENSE: MIT License. See end of file.
*/

#include "bc6hdecomp.h"

namespace bc6hdecomp {

typedef struct bcdec__bitstream {
    unsigned long long low;
    unsigned long long high;
} bcdec__bitstream_t;

static int bcdec__bitstream_read_bits(bcdec__bitstream_t* bstream, int numBits) {
    unsigned int mask = (1 << numBits) - 1;
    /* Read the low N bits */
    unsigned int bits = (bstream->low & mask);

    bstream->low >>= numBits;
    /* Put the low N bits of "high" into the high 64-N bits of "low". */
    bstream->low |= (bstream->high & mask) << (sizeof(bstream->high) * 8 - numBits);
    bstream->high >>= numBits;
    
    return bits;
}

static inline int bcdec__bitstream_read_bit(bcdec__bitstream_t* bstream) {
    return bcdec__bitstream_read_bits(bstream, 1);
}

/*  reversed bits pulling, used in BC6H decoding
    why ?? just why ??? */
static int bcdec__bitstream_read_bits_r(bcdec__bitstream_t* bstream, int numBits) {
    int bits = bcdec__bitstream_read_bits(bstream, numBits);
    /* Reverse the bits. */
    int result = 0;
    while (numBits--) {
        result <<= 1;
        result |= (bits & 1);
        bits >>= 1;
    }
    return result;
}

/* http://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend */
static int bcdec__extend_sign(int val, int bits) {
    return (val << (32 - bits)) >> (32 - bits);
}

static int bcdec__transform_inverse(int val, int a0, int bits, int isSigned) {
    /* If the precision of A0 is "p" bits, then the transform algorithm is:
       B0 = (B0 + A0) & ((1 << p) - 1) */
    val = (val + a0) & ((1 << bits) - 1);
    if (isSigned) {
        val = bcdec__extend_sign(val, bits);
    }
    return val;
}

/* pretty much copy-paste from documentation */
static int bcdec__unquantize(int val, int bits, int isSigned) {
    int unq, s = 0;

    if (!isSigned) {
        if (bits >= 15) {
            unq = val;
        } else if (!val) {
            unq = 0;
        } else if (val == ((1 << bits) - 1)) {
            unq = 0xFFFF;
        } else {
            unq = ((val << 16) + 0x8000) >> bits;
        }
    } else {
        if (bits >= 16) {
            unq = val;
        } else {
            if (val < 0) {
                s = 1;
                val = -val;
            }

            if (val == 0) {
                unq = 0;
            } else if (val >= ((1 << (bits - 1)) - 1)) {
                unq = 0x7FFF;
            } else {
                unq = ((val << 15) + 0x4000) >> (bits - 1);
            }

            if (s) {
                unq = -unq;
            }
        }
    }
    return unq;
}

static int bcdec__interpolate(int a, int b, int* weights, int index) {
    return (a * (64 - weights[index]) + b * weights[index] + 32) >> 6;
}

static unsigned short bcdec__finish_unquantize(int val, int isSigned) {
    int s;

    if (!isSigned) {
        return (unsigned short)((val * 31) >> 6);                   /* scale the magnitude by 31 / 64 */
    } else {
        val = (val < 0) ? -(((-val) * 31) >> 5) : (val * 31) >> 5;  /* scale the magnitude by 31 / 32 */
        s = 0;
        if (val < 0) {
            s = 0x8000;
            val = -val;
        }
        return (unsigned short)(s | val);
    }
}

extern "C" void bcdec_bc6h_half(const void* compressedBlock, void* decompressedBlock, int destinationPitch, int isSigned) {
    static char actual_bits_count[4][14] = {
        { 10, 7, 11, 11, 11, 9, 8, 8, 8, 6, 10, 11, 12, 16 },   /*  W */
        {  5, 6,  5,  4,  4, 5, 6, 5, 5, 6, 10,  9,  8,  4 },   /* dR */
        {  5, 6,  4,  5,  4, 5, 5, 6, 5, 6, 10,  9,  8,  4 },   /* dG */
        {  5, 6,  4,  4,  5, 5, 5, 5, 6, 6, 10,  9,  8,  4 }    /* dB */
    };

    /* There are 32 possible partition sets for a two-region tile.
       Each 4x4 block represents a single shape.
       Here also every fix-up index has MSB bit set. */
    static unsigned char partition_sets[32][4][4] = {
        { {128, 0,   1, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 0, 1, 129} },   /*  0 */
        { {128, 0,   0, 1}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 0, 129} },   /*  1 */
        { {128, 1,   1, 1}, {0, 1, 1, 1}, {  0, 1, 1, 1}, {0, 1, 1, 129} },   /*  2 */
        { {128, 0,   0, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 1, 1, 129} },   /*  3 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 1, 129} },   /*  4 */
        { {128, 0,   1, 1}, {0, 1, 1, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /*  5 */
        { {128, 0,   0, 1}, {0, 0, 1, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /*  6 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 0, 1, 1}, {0, 1, 1, 129} },   /*  7 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 1}, {0, 0, 1, 129} },   /*  8 */
        { {128, 0,   1, 1}, {0, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /*  9 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /* 10 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 1}, {0, 1, 1, 129} },   /* 11 */
        { {128, 0,   0, 1}, {0, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 12 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 13 */
        { {128, 0,   0, 0}, {1, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 14 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 0}, {1, 1, 1, 129} },   /* 15 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {  1, 1, 1, 0}, {1, 1, 1, 129} },   /* 16 */
        { {128, 1, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 0}, {0, 0, 0,   0} },   /* 17 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 1,   0} },   /* 18 */
        { {128, 1, 129, 1}, {0, 0, 1, 1}, {  0, 0, 0, 1}, {0, 0, 0,   0} },   /* 19 */
        { {128, 0, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 0}, {0, 0, 0,   0} },   /* 20 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {129, 1, 0, 0}, {1, 1, 1,   0} },   /* 21 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 0,   0} },   /* 22 */
        { {128, 1,   1, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 0, 0, 129} },   /* 23 */
        { {128, 0, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 0,   0} },   /* 24 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 0,   0} },   /* 25 */
        { {128, 1, 129, 0}, {0, 1, 1, 0}, {  0, 1, 1, 0}, {0, 1, 1,   0} },   /* 26 */
        { {128, 0, 129, 1}, {0, 1, 1, 0}, {  0, 1, 1, 0}, {1, 1, 0,   0} },   /* 27 */
        { {128, 0,   0, 1}, {0, 1, 1, 1}, {129, 1, 1, 0}, {1, 0, 0,   0} },   /* 28 */
        { {128, 0,   0, 0}, {1, 1, 1, 1}, {129, 1, 1, 1}, {0, 0, 0,   0} },   /* 29 */
        { {128, 1, 129, 1}, {0, 0, 0, 1}, {  1, 0, 0, 0}, {1, 1, 1,   0} },   /* 30 */
        { {128, 0, 129, 1}, {1, 0, 0, 1}, {  1, 0, 0, 1}, {1, 1, 0,   0} }    /* 31 */
    };

    static int aWeight3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };
    static int aWeight4[16] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };

    bcdec__bitstream_t bstream;
    int mode, partition, numPartitions, i, j, partitionSet, indexBits, index, ep_i, actualBits0Mode;
    int r[4], g[4], b[4];       /* wxyz */
    unsigned short* decompressed;
    int* weights;

    decompressed = (unsigned short*)decompressedBlock;

    bstream.low = ((unsigned long long*)compressedBlock)[0];
    bstream.high = ((unsigned long long*)compressedBlock)[1];

    r[0] = r[1] = r[2] = r[3] = 0;
    g[0] = g[1] = g[2] = g[3] = 0;
    b[0] = b[1] = b[2] = b[3] = 0;

    mode = bcdec__bitstream_read_bits(&bstream, 2);
    if (mode > 1) {
        mode |= (bcdec__bitstream_read_bits(&bstream, 3) << 2);
    }

    /* modes >= 11 (10 in my code) are using 0 one, others will read it from the bitstream */
    partition = 0;

    switch (mode) {
        /* mode 1 */
        case 0b00: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 75 bits (10.555, 10.555, 10.555) */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 0;
        } break;

        /* mode 2 */
        case 0b01: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 75 bits (7666, 7666, 7666) */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* rw[6:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* gw[6:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* bw[6:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 1;
        } break;

        /* mode 3 */
        case 0b00010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.555, 11.444, 11.444) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 2;
        } break;

        /* mode 4 */
        case 0b00110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.444, 11.555, 11.444) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* ry[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rz[3:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 3;
        } break;

        /* mode 5 */
        case 0b01010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.444, 11.444, 11.555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* ry[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rz[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */ 
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 4;
        } break;

        /* mode 6 */
        case 0b01110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (9555, 9555, 9555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* rw[8:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* gw[8:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* bw[8:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 5;
        } break;

        /* mode 7 */
        case 0b10010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8666, 8555, 8555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 6;
        } break;

        /* mode 8 */
        case 0b10110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8555, 8666, 8555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* zx[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 7;
        } break;

        /* mode 9 */
        case 0b11010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8555, 8555, 8666) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bw[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 8;
        } break;

        /* mode 10 */
        case 0b11110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (6666, 6666, 6666) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rw[5:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gw[5:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bw[5:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 9;
        } break;

        /* mode 11 */
        case 0b00011: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (10.10, 10.10, 10.10) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rx[9:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gx[9:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bx[9:0] */
            mode = 10;
        } break;

        /* mode 12 */
        case 0b00111: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (11.9, 11.9, 11.9) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* rx[8:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* gx[8:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* bx[8:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            mode = 11;
        } break;

        /* mode 13 */
        case 0b01011: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (12.8, 12.8, 12.8) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rx[7:0] */
            r[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* rx[10:11] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gx[7:0] */
            g[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* gx[10:11] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bx[7:0] */
            b[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* bx[10:11] */
            mode = 12;
        } break;

        /* mode 14 */
        case 0b01111: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (16.4, 16.4, 16.4) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* rw[10:15] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* gw[10:15] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* bw[10:15] */
            mode = 13;
        } break;

        default: {
            /* Modes 10011, 10111, 11011, and 11111 (not shown) are reserved.
               Do not use these in your encoder. If the hardware is passed blocks
               with one of these modes specified, the resulting decompressed block
               must contain all zeroes in all channels except for the alpha channel. */
            for (i = 0; i < 4; ++i) {
                for (j = 0; j < 4; ++j) {
                    decompressed[j * 3 + 0] = 0;
                    decompressed[j * 3 + 1] = 0;
                    decompressed[j * 3 + 2] = 0;
                }
                decompressed += destinationPitch;
            }

            return;
        }
    }

    numPartitions = (mode >= 10) ? 0 : 1;

    actualBits0Mode = actual_bits_count[0][mode];
    if (isSigned) {
        r[0] = bcdec__extend_sign(r[0], actualBits0Mode);
        g[0] = bcdec__extend_sign(g[0], actualBits0Mode);
        b[0] = bcdec__extend_sign(b[0], actualBits0Mode);
    }

    /* Mode 11 (like Mode 10) does not use delta compression,
       and instead stores both color endpoints explicitly.  */
    if ((mode != 9 && mode != 10) || isSigned) {
        for (i = 1; i < (numPartitions + 1) * 2; ++i) {
            r[i] = bcdec__extend_sign(r[i], actual_bits_count[1][mode]);
            g[i] = bcdec__extend_sign(g[i], actual_bits_count[2][mode]);
            b[i] = bcdec__extend_sign(b[i], actual_bits_count[3][mode]);
        }
    }

    if (mode != 9 && mode != 10) {
        for (i = 1; i < (numPartitions + 1) * 2; ++i) {
            r[i] = bcdec__transform_inverse(r[i], r[0], actualBits0Mode, isSigned);
            g[i] = bcdec__transform_inverse(g[i], g[0], actualBits0Mode, isSigned);
            b[i] = bcdec__transform_inverse(b[i], b[0], actualBits0Mode, isSigned);
        }
    }

    for (i = 0; i < (numPartitions + 1) * 2; ++i) {
        r[i] = bcdec__unquantize(r[i], actualBits0Mode, isSigned);
        g[i] = bcdec__unquantize(g[i], actualBits0Mode, isSigned);
        b[i] = bcdec__unquantize(b[i], actualBits0Mode, isSigned);
    }

    weights = (mode >= 10) ? aWeight4 : aWeight3;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            partitionSet = (mode >= 10) ? ((i|j) ? 0 : 128) : partition_sets[partition][i][j];

            indexBits = (mode >= 10) ? 4 : 3;
            /* fix-up index is specified with one less bit */
            /* The fix-up index for subset 0 is always index 0 */
            if (partitionSet & 0x80) {
                indexBits--;
            }
            partitionSet &= 0x01;

            index = bcdec__bitstream_read_bits(&bstream, indexBits);

            ep_i = partitionSet * 2;
            decompressed[j * 3 + 0] = bcdec__finish_unquantize(
                                            bcdec__interpolate(r[ep_i], r[ep_i+1], weights, index), isSigned);
            decompressed[j * 3 + 1] = bcdec__finish_unquantize(
                                            bcdec__interpolate(g[ep_i], g[ep_i+1], weights, index), isSigned);
            decompressed[j * 3 + 2] = bcdec__finish_unquantize(
                                            bcdec__interpolate(b[ep_i], b[ep_i+1], weights, index), isSigned);
        }

        decompressed += destinationPitch;
    }
}

}  // namespace bc6hdecomp

/* MIT License

Copyright (c) 2022 Sergii Kudlai

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
