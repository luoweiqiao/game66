#ifndef _ed2k_hash_md4_h_included_
#define _ed2k_hash_md4_h_included_

#include "fundamental/common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EMULE_MD4_BLOCKSIZE	(9500*1024)


#define SLASH_CHAR	'/'


typedef struct
{
  uint32_t state[4];                                   /* state (ABCD) */
  uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */
  uint8_t buffer[64];                         /* input buffer */
} MD4_CTX;

void MD4Init(MD4_CTX *);
void MD4Update(MD4_CTX *, uint8_t *, uint32_t);
void MD4Final(uint8_t a[16], MD4_CTX * b);

#ifdef __cplusplus
}
#endif

#endif	/* ifndef _ed2k_hash_md4_h_included_ */


