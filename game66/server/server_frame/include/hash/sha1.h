/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */
/* This code is in the public domain */
/* $Id: sha1.h,v 1.1 2004/05/05 09:11:39 hadess Exp $ */

#ifndef SHA_H
#define SHA_H


#include "fundamental/common.h"
#include <string>


#define SHA_BLOCKSIZE		64
#define SHA_DIGESTSIZE		20

struct SHA_INFO{
    uint32_t digest[5];		/* message digest */
    uint32_t count_lo, count_hi;	/* 64-bit bit count */
    uint8_t data[SHA_BLOCKSIZE];	/* SHA data buffer */
    int local;			/* unprocessed amount in data */
};

void sha_init(SHA_INFO *);
void sha_update(SHA_INFO *, uint8_t *, int32_t);
void sha_final(uint8_t [20], SHA_INFO *);

void sha_stream(uint8_t [20], SHA_INFO *, FILE *);
void sha_print(uint8_t [20]);
std::string sha_version(void);

#define SHA_VERSION 1


#define SHA_BYTE_ORDER 1234

#endif /* SHA_H */
