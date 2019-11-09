#include "crypt/crypt.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <arpa/inet.h>
using namespace svrlib;
using namespace std;

const int MD5_LBLOCK = 16;
struct MD5_CTX
{
	uint32_t A,B,C,D;
	uint32_t Nl,Nh;
	uint32_t data[MD5_LBLOCK];
	int num;
};

static void InitRand()
{
	srand( (unsigned int )time(0));
}

const int QWORD_MAX = 65534;
static uint16_t GetRand()
{
	return rand()%QWORD_MAX;
}

#ifndef L_ENDIAN
#define L_ENDIAN
#endif




/*
	BOOL Md5Test();
	void Md5HashBuffer( QBYTE *outBuffer, QBYTE *inBuffer, int length);
*/


/*
***********************************************************************************************
	MD5数据结构
***********************************************************************************************
*/


/*
	MD5初始化
*/

#define INIT_DATA_A (uint32_t)0x67452301L
#define INIT_DATA_B (uint32_t)0xefcdab89L
#define INIT_DATA_C (uint32_t)0x98badcfeL
#define INIT_DATA_D (uint32_t)0x10325476L

static void MD5_Init(MD5_CTX *c)
{
	memset(c,0,sizeof(MD5_CTX));
	c->A=INIT_DATA_A;
	c->B=INIT_DATA_B;
	c->C=INIT_DATA_C;
	c->D=INIT_DATA_D;
	c->Nl=0;
	c->Nh=0;
	c->num=0;
}



/*
	MD5更新
*/
const uint32_t MD5_CBLOCK = 64;

#define c2l(c,l)	(l = ((uint32_t)(*((c)++))), l|=(((uint32_t)(*((c)++)))<< 8), l|=(((uint32_t)(*((c)++)))<<16), l|=(((uint32_t)(*((c)++)))<<24))

#define p_c2l(c,l,n)	{ switch (n) { case 0: l =((uint32_t)(*((c)++))); case 1: l|=((uint32_t)(*((c)++)))<< 8; case 2: l|=((uint32_t)(*((c)++)))<<16; case 3: l|=((uint32_t)(*((c)++)))<<24; } }

#define p_c2l_p(c,l,sc,len) { switch (sc) { case 0: l =((uint32_t)(*((c)++))); if (--len == 0) break; case 1: l|=((uint32_t)(*((c)++)))<< 8; if (--len == 0) break; case 2: l|=((uint32_t)(*((c)++)))<<16; } }

#define c2l_p(c,l,n)	{ l=0; (c)+=n; switch (n) { case 3: l =((uint32_t)(*(--(c))))<<16; case 2: l|=((uint32_t)(*(--(c))))<< 8; case 1: l|=((uint32_t)(*(--(c))))    ; } }


#define	F(b,c,d)	((((c) ^ (d)) & (b)) ^ (d))
#define	G(b,c,d)	((((b) ^ (c)) & (d)) ^ (c))
#define	H(b,c,d)	((b) ^ (c) ^ (d))
#define	I(b,c,d)	(((~(d)) | (b)) ^ (c))





#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffffL)>>(32-(n))))



#define R0(a,b,c,d,k,s,t) { a+=((k)+(t)+F((b),(c),(d))); a=ROTATE(a,s); a+=b; };
#define R1(a,b,c,d,k,s,t) { a+=((k)+(t)+G((b),(c),(d))); a=ROTATE(a,s); a+=b; };

#define R2(a,b,c,d,k,s,t) { a+=((k)+(t)+H((b),(c),(d))); a=ROTATE(a,s); a+=b; };

#define R3(a,b,c,d,k,s,t) { a+=((k)+(t)+I((b),(c),(d))); a=ROTATE(a,s); a+=b; };

#ifndef MD5_ASM

static void md5_block(MD5_CTX *c, register uint32_t *X, int num)
{
	register uint32_t A,B,C,D;
	
	A=c->A;
	B=c->B;
	C=c->C;
	D=c->D;
	for (;;)
	{
		num-=64;
		if (num < 0) break;
		/* Round 0 */
		R0(A,B,C,D,X[ 0], 7,0xd76aa478L);
		R0(D,A,B,C,X[ 1],12,0xe8c7b756L);
		R0(C,D,A,B,X[ 2],17,0x242070dbL);
		R0(B,C,D,A,X[ 3],22,0xc1bdceeeL);
		R0(A,B,C,D,X[ 4], 7,0xf57c0fafL);
		R0(D,A,B,C,X[ 5],12,0x4787c62aL);
		R0(C,D,A,B,X[ 6],17,0xa8304613L);
		R0(B,C,D,A,X[ 7],22,0xfd469501L);
		R0(A,B,C,D,X[ 8], 7,0x698098d8L);
		R0(D,A,B,C,X[ 9],12,0x8b44f7afL);
		R0(C,D,A,B,X[10],17,0xffff5bb1L);
		R0(B,C,D,A,X[11],22,0x895cd7beL);
		R0(A,B,C,D,X[12], 7,0x6b901122L);
		R0(D,A,B,C,X[13],12,0xfd987193L);
		R0(C,D,A,B,X[14],17,0xa679438eL);
		R0(B,C,D,A,X[15],22,0x49b40821L);
		/* Round 1 */
		R1(A,B,C,D,X[ 1], 5,0xf61e2562L);
		R1(D,A,B,C,X[ 6], 9,0xc040b340L);
		R1(C,D,A,B,X[11],14,0x265e5a51L);
		R1(B,C,D,A,X[ 0],20,0xe9b6c7aaL);
		R1(A,B,C,D,X[ 5], 5,0xd62f105dL);
		R1(D,A,B,C,X[10], 9,0x02441453L);
		R1(C,D,A,B,X[15],14,0xd8a1e681L);
		R1(B,C,D,A,X[ 4],20,0xe7d3fbc8L);
		R1(A,B,C,D,X[ 9], 5,0x21e1cde6L);
		R1(D,A,B,C,X[14], 9,0xc33707d6L);
		R1(C,D,A,B,X[ 3],14,0xf4d50d87L);
		R1(B,C,D,A,X[ 8],20,0x455a14edL);
		R1(A,B,C,D,X[13], 5,0xa9e3e905L);
		R1(D,A,B,C,X[ 2], 9,0xfcefa3f8L);
		R1(C,D,A,B,X[ 7],14,0x676f02d9L);
		R1(B,C,D,A,X[12],20,0x8d2a4c8aL);
		/* Round 2 */
		R2(A,B,C,D,X[ 5], 4,0xfffa3942L);
		R2(D,A,B,C,X[ 8],11,0x8771f681L);
		R2(C,D,A,B,X[11],16,0x6d9d6122L);
		R2(B,C,D,A,X[14],23,0xfde5380cL);
		R2(A,B,C,D,X[ 1], 4,0xa4beea44L);
		R2(D,A,B,C,X[ 4],11,0x4bdecfa9L);
		R2(C,D,A,B,X[ 7],16,0xf6bb4b60L);
		R2(B,C,D,A,X[10],23,0xbebfbc70L);
		R2(A,B,C,D,X[13], 4,0x289b7ec6L);
		R2(D,A,B,C,X[ 0],11,0xeaa127faL);
		R2(C,D,A,B,X[ 3],16,0xd4ef3085L);
		R2(B,C,D,A,X[ 6],23,0x04881d05L);
		R2(A,B,C,D,X[ 9], 4,0xd9d4d039L);
		R2(D,A,B,C,X[12],11,0xe6db99e5L);
		R2(C,D,A,B,X[15],16,0x1fa27cf8L);
		R2(B,C,D,A,X[ 2],23,0xc4ac5665L);
		/* Round 3 */
		R3(A,B,C,D,X[ 0], 6,0xf4292244L);
		R3(D,A,B,C,X[ 7],10,0x432aff97L);
		R3(C,D,A,B,X[14],15,0xab9423a7L);
		R3(B,C,D,A,X[ 5],21,0xfc93a039L);
		R3(A,B,C,D,X[12], 6,0x655b59c3L);
		R3(D,A,B,C,X[ 3],10,0x8f0ccc92L);
		R3(C,D,A,B,X[10],15,0xffeff47dL);
		R3(B,C,D,A,X[ 1],21,0x85845dd1L);
		R3(A,B,C,D,X[ 8], 6,0x6fa87e4fL);
		R3(D,A,B,C,X[15],10,0xfe2ce6e0L);
		R3(C,D,A,B,X[ 6],15,0xa3014314L);
		R3(B,C,D,A,X[13],21,0x4e0811a1L);
		R3(A,B,C,D,X[ 4], 6,0xf7537e82L);
		R3(D,A,B,C,X[11],10,0xbd3af235L);
		R3(C,D,A,B,X[ 2],15,0x2ad7d2bbL);
		R3(B,C,D,A,X[ 9],21,0xeb86d391L);
		
		A+=c->A&0xffffffffL;
		B+=c->B&0xffffffffL;
		c->A=A;
		c->B=B;
		C+=c->C&0xffffffffL;
		D+=c->D&0xffffffffL;
		c->C=C;
		c->D=D;
		X+=16;
	}
}
#endif


static void MD5_Update(MD5_CTX *c, const register unsigned char *data, uint32_t len)
{
	register uint32_t *p;
	int sw,sc;
	uint32_t l;
	
	if (len == 0) return;
	
	l=(c->Nl+(len<<3))&0xffffffffL;
	/* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
	* Wei Dai <weidai@eskimo.com> for pointing it out. */
	if (l < c->Nl) /* overflow */
		c->Nh++;
	c->Nh+=(len>>29);
	c->Nl=l;
	
	if (c->num != 0)
	{
		p=c->data;
		sw=c->num>>2;
		sc=c->num&0x03;
		
		if ((c->num+len) >= MD5_CBLOCK)
		{
			l= p[sw];
			p_c2l(data,l,sc);
			p[sw++]=l;
			for (; sw<MD5_LBLOCK; sw++)
			{
				c2l(data,l);
				p[sw]=l;
			}
			len-=(MD5_CBLOCK-c->num);
			
			md5_block(c,p,64);
			c->num=0;
			/* drop through and do the rest */
		}
		else
		{
			int ew,ec;
			
			c->num+=(int)len;
			if ((sc+len) < 4) /* ugly, add char's to a word */
			{
				l= p[sw];
				p_c2l_p(data,l,sc,len);
				p[sw]=l;
			}
			else
			{
				ew=(c->num>>2);
				ec=(c->num&0x03);
				l= p[sw];
				p_c2l(data,l,sc);
				p[sw++]=l;
				for (; sw < ew; sw++)
				{ c2l(data,l); p[sw]=l; }
				if (ec)
				{
					c2l_p(data,l,ec);
					p[sw]=l;
				}
			}
			return;
		}
	}

	/* we now can process the input data in blocks of MD5_CBLOCK
	* chars and save the leftovers to c->data. */
#ifdef L_ENDIAN
	if ((((uint64_t)data)%sizeof(uint32_t)) == 0)
	{
		sw=len/MD5_CBLOCK;
		if (sw > 0)
		{
			sw*=MD5_CBLOCK;
			md5_block(c,(uint32_t *)data,sw);
			data+=sw;
			len-=sw;
		}
	}
#endif
	p=c->data;
	while (len >= MD5_CBLOCK)
	{
#if defined(L_ENDIAN) || defined(B_ENDIAN)
		if (p != (uint32_t *)data)
			memcpy(p,data,MD5_CBLOCK);
		data+=MD5_CBLOCK;
#ifdef B_ENDIAN
		for (sw=(MD5_LBLOCK/4); sw; sw--)
		{
			Endian_Reverse32(p[0]);
			Endian_Reverse32(p[1]);
			Endian_Reverse32(p[2]);
			Endian_Reverse32(p[3]);
			p+=4;
		}
#endif
#else
		for (sw=(MD5_LBLOCK/4); sw; sw--)
		{
			c2l(data,l); *(p++)=l;
			c2l(data,l); *(p++)=l;
			c2l(data,l); *(p++)=l;
			c2l(data,l); *(p++)=l; 
		} 
#endif
		p=c->data;
		md5_block(c,p,64);
		len-=MD5_CBLOCK;
	}
	sc=(int)len;
	c->num=sc;
	if (sc)
	{
		sw=sc>>2;	/* words to copy */
#ifdef L_ENDIAN
		p[sw]=0;
		memcpy(p,data,sc);
#else
		sc&=0x03;
		for ( ; sw; sw--)
		{ c2l(data,l); *(p++)=l; }
		c2l_p(data,l,sc);
		*p=l;
#endif
	}
}



/*
	MD5结束
*/
#define MD5_LAST_BLOCK  56

#define l2c(l,c)	(*((c)++)=(unsigned char)(((l)     )&0xff), *((c)++)=(unsigned char)(((l)>> 8L)&0xff), *((c)++)=(unsigned char)(((l)>>16L)&0xff), *((c)++)=(unsigned char)(((l)>>24L)&0xff))


static void MD5_Final(unsigned char *md, MD5_CTX *c)
{
	register int i,j;
	register uint32_t l;
	register uint32_t *p;
	static unsigned char end[4]={0x80,0x00,0x00,0x00};
	unsigned char *cp=end;
	
	/* c->num should definitly have room for at least one more byte. */
	p=c->data;
	j=c->num;
	i=j>>2;
	
	/* purify often complains about the following line as an
	* Uninitialized Memory Read.  While this can be true, the
	* following p_c2l macro will reset l when that case is true.
	* This is because j&0x03 contains the number of 'valid' bytes
	* already in p[i].  If and only if j&0x03 == 0, the UMR will
	* occur but this is also the only time p_c2l will do
	* l= *(cp++) instead of l|= *(cp++)
	* Many thanks to Alex Tang <altitude@cic.net> for pickup this
	* 'potential bug' */
#ifdef PURIFY
	if ((j&0x03) == 0) p[i]=0;
#endif
	l=p[i];
	p_c2l(cp,l,j&0x03);
	p[i]=l;
	i++;
	/* i is the next 'undefined word' */
	if (c->num >= MD5_LAST_BLOCK)
	{
		for (; i<MD5_LBLOCK; i++)
			p[i]=0;
		md5_block(c,p,64);
		i=0;
	}
	for (; i<(MD5_LBLOCK-2); i++)
		p[i]=0;
	p[MD5_LBLOCK-2]=c->Nl;
	p[MD5_LBLOCK-1]=c->Nh;
	md5_block(c,p,64);
	cp=md;
	l=c->A; l2c(l,cp);
	l=c->B; l2c(l,cp);
	l=c->C; l2c(l,cp);
	l=c->D; l2c(l,cp);
	
	/* clear stuff, md5_block may be leaving some stuff on the stack
	* but I'm not worried :-) */
	c->num=0;
	/*	memset((char *)&c,0,sizeof(c));*/
}


#define SALT_LEN 2
#define ZERO_LEN 7

#define ROUNDS 16
#define LOG_ROUNDS 4
const uint32_t DELTA = 0x9e3779b9;
namespace CryptFunc
{
static void TeaEncryptECB(const unsigned char *pInBuf, const unsigned char *pKey, unsigned char *pOutBuf)
{
	uint32_t y, z;
	uint32_t sum;
	uint32_t k[4];
	int i;

	/*plain-text is TCP/IP-endian;*/

	/*GetBlockBigEndian(in, y, z);*/
	y = ntohl(*((uint32_t*)pInBuf));
	z = ntohl(*((uint32_t*)(pInBuf+4)));
	/*TCP/IP network byte order (which is big-endian).*/

	for ( i = 0; i<4; i++)
	{
		/*now key is TCP/IP-endian;*/
		k[i] = ntohl(*((uint32_t*)(pKey+i*4)));
	}

	sum = 0;
	for (i=0; i<ROUNDS; i++)
	{   
		sum += DELTA;
		y += (z << 4) + (k[0] ^ z) + (sum ^ (z >> 5)) + k[1];
		z += (y << 4) + (k[2] ^ y) + (sum ^ (y >> 5)) + k[3];
	}



	*((uint32_t*)pOutBuf) = htonl(y);
	*((uint32_t*)(pOutBuf+4)) = htonl(z);
	

	/*now encrypted buf is TCP/IP-endian;*/
}

static void TeaDecryptECB(const unsigned char *pInBuf, const unsigned char *pKey, unsigned char *pOutBuf)
{
	uint32_t y, z, sum;
	uint32_t k[4];
	int i;

	/*now encrypted buf is TCP/IP-endian;*/
	/*TCP/IP network byte order (which is big-endian).*/
	y = ntohl(*((uint32_t*)pInBuf));
	z = ntohl(*((uint32_t*)(pInBuf+4)));

	for ( i=0; i<4; i++)
	{
		/*key is TCP/IP-endian;*/
		k[i] = ntohl(*((uint32_t*)(pKey+i*4)));
	}

	sum = DELTA << LOG_ROUNDS;
	for (i=0; i<ROUNDS; i++)
	{
		z -= (y << 4) + (k[2] ^ y) + (sum ^ (y >> 5)) + k[3]; 
		y -= (z << 4) + (k[0] ^ z) + (sum ^ (z >> 5)) + k[1];
		sum -= DELTA;
	}

	*((uint32_t*)pOutBuf) = htonl(y);
	*((uint32_t*)(pOutBuf+4)) = htonl(z);

	/*now plain-text is TCP/IP-endian;*/
}


/* ///////////////////////////////////////////////////////////////////////////////////////////// */

/*pKey为16byte*/
/*
	输入:nInBufLen为需加密的明文部分(Body)长度;
	输出:返回为加密后的长度(是8byte的倍数);
*/
/*TEA加密算法,CBC模式*/
/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
int oi_symmetry_encrypt2_len(int nInBufLen)
{
	
	int nPadSaltBodyZeroLen/*PadLen(1byte)+Salt+Body+Zero的长度*/;
	int nPadlen;

	/*根据Body长度计算PadLen,最小必需长度必需为8byte的整数倍*/
	nPadSaltBodyZeroLen = nInBufLen/*Body长度*/+1+SALT_LEN+ZERO_LEN/*PadLen(1byte)+Salt(2byte)+Zero(7byte)*/;
	if((nPadlen=nPadSaltBodyZeroLen%8) != 0) /*len=nSaltBodyZeroLen%8*/
	{
		/*模8余0需补0,余1补7,余2补6,...,余7补1*/
		nPadlen=8-nPadlen;
	}

	return nPadSaltBodyZeroLen+nPadlen;
}


/*pKey为16byte*/
/*
	输入:pInBuf为需加密的明文部分(Body),nInBufLen为pInBuf长度;
	输出:pOutBuf为密文格式,pOutBufLen为pOutBuf的长度是8byte的倍数;
*/
/*TEA加密算法,CBC模式*/
/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
void oi_symmetry_encrypt2(const unsigned char* pInBuf, int nInBufLen, const unsigned char* pKey, unsigned char* pOutBuf, int *pOutBufLen)
{
	
	int nPadSaltBodyZeroLen/*PadLen(1byte)+Salt+Body+Zero的长度*/;
	int nPadlen;
	unsigned char src_buf[8], iv_plain[8], *iv_crypt;
	int src_i, i, j;

	/*根据Body长度计算PadLen,最小必需长度必需为8byte的整数倍*/
	nPadSaltBodyZeroLen = nInBufLen/*Body长度*/+1+SALT_LEN+ZERO_LEN/*PadLen(1byte)+Salt(2byte)+Zero(7byte)*/;
	if((nPadlen=nPadSaltBodyZeroLen%8) != 0) /*len=nSaltBodyZeroLen%8*/
	{
		/*模8余0需补0,余1补7,余2补6,...,余7补1*/
		nPadlen=8-nPadlen;
	}

	/*srand( (unsigned)time( NULL ) ); 初始化随机数*/
	/*加密第一块数据(8byte),取前面10byte*/
	src_buf[0] = (((unsigned char)rand()) & 0x0f8)/*最低三位存PadLen,清零*/ | (unsigned char)nPadlen;
	src_i = 1; /*src_i指向src_buf下一个位置*/

	while(nPadlen--)
		src_buf[src_i++]=(unsigned char)rand(); /*Padding*/

	/*come here, src_i must <= 8*/

	for ( i=0; i<8; i++)
		iv_plain[i] = 0;
	iv_crypt = iv_plain; /*make zero iv*/

	*pOutBufLen = 0; /*init OutBufLen*/

	for (i=1;i<=SALT_LEN;) /*Salt(2byte)*/
	{
		if (src_i<8)
		{
			src_buf[src_i++]=(unsigned char)rand();
			i++; /*i inc in here*/
		}

		if (src_i==8)
		{
			/*src_i==8*/

			for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
				src_buf[j]^=iv_crypt[j];

			/*pOutBuffer、pInBuffer均为8byte, pKey为16byte*/
			/*加密*/
			TeaEncryptECB(src_buf, pKey, pOutBuf);

			for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
				pOutBuf[j]^=iv_plain[j];

			/*保存当前的iv_plain*/
			for (j=0;j<8;j++)
				iv_plain[j]=src_buf[j];

			/*更新iv_crypt*/
			src_i=0;
			iv_crypt=pOutBuf;
			*pOutBufLen+=8;
			pOutBuf+=8;
		}
	}

	/*src_i指向src_buf下一个位置*/

	while(nInBufLen)
	{
		if (src_i<8)
		{
			src_buf[src_i++]=*(pInBuf++);
			nInBufLen--;
		}

		if (src_i==8)
		{
			/*src_i==8*/
			
			for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
				src_buf[j]^=iv_crypt[j];
			/*pOutBuffer、pInBuffer均为8byte, pKey为16byte*/
			TeaEncryptECB(src_buf, pKey, pOutBuf);

			for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
				pOutBuf[j]^=iv_plain[j];

			/*保存当前的iv_plain*/
			for (j=0;j<8;j++)
				iv_plain[j]=src_buf[j];

			src_i=0;
			iv_crypt=pOutBuf;
			*pOutBufLen+=8;
			pOutBuf+=8;
		}
	}

	/*src_i指向src_buf下一个位置*/

	for (i=1;i<=ZERO_LEN;)
	{
		if (src_i<8)
		{
			src_buf[src_i++]=0;
			i++; /*i inc in here*/
		}

		if (src_i==8)
		{
			/*src_i==8*/
			
			for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
				src_buf[j]^=iv_crypt[j];
			/*pOutBuffer、pInBuffer均为8byte, pKey为16byte*/
			TeaEncryptECB(src_buf, pKey, pOutBuf);

			for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
				pOutBuf[j]^=iv_plain[j];

			/*保存当前的iv_plain*/
			for (j=0;j<8;j++)
				iv_plain[j]=src_buf[j];

			src_i=0;
			iv_crypt=pOutBuf;
			*pOutBufLen+=8;
			pOutBuf+=8;
		}
	}

}


/*pKey为16byte*/
/*
	输入:pInBuf为密文格式,nInBufLen为pInBuf的长度是8byte的倍数; *pOutBufLen为接收缓冲区的长度
		特别注意*pOutBufLen应预置接收缓冲区的长度!
	输出:pOutBuf为明文(Body),pOutBufLen为pOutBuf的长度,至少应预留nInBufLen-10;
	返回值:如果格式正确返回TRUE;
*/
/*TEA解密算法,CBC模式*/
/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
bool oi_symmetry_decrypt2(const unsigned char* pInBuf, int nInBufLen, const unsigned char* pKey, unsigned char* pOutBuf, int *pOutBufLen)
{

	int nPadLen, nPlainLen;
	unsigned char dest_buf[8], zero_buf[8];
	const unsigned char *iv_pre_crypt, *iv_cur_crypt;
	int dest_i, i, j;
	const unsigned char *pInBufBoundary;
	int nBufPos;
	nBufPos = 0;


	
	if ((nInBufLen%8) || (nInBufLen<16)) return false;
	

	TeaDecryptECB(pInBuf, pKey, dest_buf);

	nPadLen = dest_buf[0] & 0x7/*只要最低三位*/;

	/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
	i = nInBufLen-1/*PadLen(1byte)*/-nPadLen-SALT_LEN-ZERO_LEN; /*明文长度*/
	if ((*pOutBufLen<i) || (i<0)) return false;
	*pOutBufLen = i;
	
	pInBufBoundary = pInBuf + nInBufLen; /*输入缓冲区的边界，下面不能pInBuf>=pInBufBoundary*/

	
	for ( i=0; i<8; i++)
		zero_buf[i] = 0;

	iv_pre_crypt = zero_buf;
	iv_cur_crypt = pInBuf; /*init iv*/

	pInBuf += 8;
	nBufPos += 8;

	dest_i=1; /*dest_i指向dest_buf下一个位置*/


	/*把Padding滤掉*/
	dest_i+=nPadLen;

	/*dest_i must <=8*/

	/*把Salt滤掉*/
	for (i=1; i<=SALT_LEN;)
	{
		if (dest_i<8)
		{
			dest_i++;
			i++;
		}
		else if (dest_i==8)
		{
			/*解开一个新的加密块*/

			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf; 

			/*异或前一块明文(在dest_buf[]中)*/
			for (j=0; j<8; j++)
			{
				if( (nBufPos + j) >= nInBufLen)
					return false;
				dest_buf[j]^=pInBuf[j];
			}

			/*dest_i==8*/
			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

			
			pInBuf += 8;
			nBufPos += 8;
	
			dest_i=0; /*dest_i指向dest_buf下一个位置*/
		}
	}

	/*还原明文*/

	nPlainLen=*pOutBufLen;
	while (nPlainLen)
	{
		if (dest_i<8)
		{
			*(pOutBuf++)=dest_buf[dest_i]^iv_pre_crypt[dest_i];
			dest_i++;
			nPlainLen--;
		}
		else if (dest_i==8)
		{
			/*dest_i==8*/

			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf; 

			/*解开一个新的加密块*/

			/*异或前一块明文(在dest_buf[]中)*/
			for (j=0; j<8; j++)
			{
				if( (nBufPos + j) >= nInBufLen)
					return false;
				dest_buf[j]^=pInBuf[j];
			}

			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/
		
			
			pInBuf += 8;
			nBufPos += 8;
	
			dest_i=0; /*dest_i指向dest_buf下一个位置*/
		}
	}

	/*校验Zero*/
	for (i=1;i<=ZERO_LEN;)
	{
		if (dest_i<8)
		{
			if(dest_buf[dest_i]^iv_pre_crypt[dest_i]) return false;
			dest_i++;
			i++;
		}
		else if (dest_i==8)
		{
			/*改变前一个加密块的指针*/
			iv_pre_crypt = iv_cur_crypt;
			iv_cur_crypt = pInBuf; 

			/*解开一个新的加密块*/

			/*异或前一块明文(在dest_buf[]中)*/
			for (j=0; j<8; j++)
			{
				if( (nBufPos + j) >= nInBufLen)
					return false;
				dest_buf[j]^=pInBuf[j];
			}

			TeaDecryptECB(dest_buf, pKey, dest_buf);

			/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

			
			pInBuf += 8;
			nBufPos += 8;
			dest_i=0; /*dest_i指向dest_buf下一个位置*/
		}
	
	}

	return true;
}



/* ///////////////////////////////////////////////////////////////////////////////////////////// */

/*pKey为16byte*/
/*
	输入:nInBufLen为需加密的明文部分(Body)长度;
	输出:返回为加密后的长度(是8byte的倍数);
*/
/*TEA加密算法,CBC模式*/
/*密文格式:PadLen(1byte)+Padding(var,0-7byte)+Salt(2byte)+Body(var byte)+Zero(7byte)*/
int qq_symmetry_encrypt3_len(int nInBufLen)
{
	
	int nPadSaltBodyZeroLen/*PadLen(1byte)+Salt+Body+Zero的长度*/;
	int nPadlen;

	/*根据Body长度计算PadLen,最小必需长度必需为8byte的整数倍*/
	nPadSaltBodyZeroLen = nInBufLen/*Body长度*/+1+SALT_LEN+ZERO_LEN/*PadLen(1byte)+Salt(2byte)+Zero(7byte)*/;
	if((nPadlen=nPadSaltBodyZeroLen%8) != 0) /*len=nSaltBodyZeroLen%8*/
	{
		/*模8余0需补0,余1补7,余2补6,...,余7补1*/
		nPadlen=8-nPadlen;
	}

	return nPadSaltBodyZeroLen+nPadlen;
}
}; //end namespace CryptFunc
CCrypt::CCrypt()
: m_dwUin(0)
{
	m_nEncryptArith = CRYPT_3;
	m_nDecryptArith = CRYPT_2;
}

CCrypt::~CCrypt()
{

}

CCrypt::CCrypt(unsigned char *pKey,unsigned char nEncryptArith,unsigned char nDecryptArith)
: m_dwUin(0)
{
	m_nEncryptArith = nEncryptArith;
	m_nDecryptArith = nDecryptArith;
	memcpy(m_arKey,pKey,SESSION_KEY_SIZE);
	InitRand();
}


void CCrypt::Encrypt(const unsigned char* pInBuf, int nInBufLen, unsigned char* pOutBuf, int& nOutBufLen)
{
	if(m_nEncryptArith ==CRYPT_2)	
	{
		int nPadSaltBodyZeroLen/*PadLen(1byte)+Salt+Body+Zero的长度*/;
		int nPadlen;
		unsigned char src_buf[8], iv_plain[8], *iv_crypt;
		int src_i, i, j;

		/*根据Body长度计算PadLen,最小必需长度必需为8byte的整数倍*/
		nPadSaltBodyZeroLen = nInBufLen/*Body长度*/+1+SALT_LEN+ZERO_LEN/*PadLen(1byte)+Salt(2byte)+Zero(7byte)*/;
		if((nPadlen=nPadSaltBodyZeroLen%8) != 0) /*len=nSaltBodyZeroLen%8*/
		{
			/*模8余0需补0,余1补7,余2补6,...,余7补1*/
			nPadlen=8-nPadlen;
		}

		/*srand( (unsigned)time( NULL ) ); 初始化随机数*/
		/*加密第一块数据(8byte),取前面10byte*/
		src_buf[0] = (((unsigned char)GetRand()) & 0x0f8/*最低三位存PadLen,清零*/) | (unsigned char)nPadlen;
		src_i = 1; /*src_i指向src_buf下一个位置*/

		while(nPadlen--)
			src_buf[src_i++]=(unsigned char)GetRand(); /*Padding*/

		/*come here, src_i must <= 8*/

		for ( i=0; i<8; i++)
			iv_plain[i] = 0;
		iv_crypt = iv_plain; /*make zero iv*/

		nOutBufLen = 0; /*init OutBufLen*/

		for (i=1;i<=SALT_LEN;) /*Salt(2byte)*/
		{
			if (src_i<8)
			{
				src_buf[src_i++]=(unsigned char)GetRand();
				i++; /*i inc in here*/
			}

			if (src_i==8)
			{
				/*src_i==8*/

				for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
					src_buf[j]^=iv_crypt[j];

				/*pOutBuffer、pInBuffer均为8byte, &m_arKey为16byte*/
				/*加密*/
				CryptFunc::TeaEncryptECB(src_buf, m_arKey, pOutBuf);

				for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
					pOutBuf[j]^=iv_plain[j];

				/*保存当前的iv_plain*/
				for (j=0;j<8;j++)
					iv_plain[j]=src_buf[j];

				/*更新iv_crypt*/
				src_i=0;
				iv_crypt=pOutBuf;
				nOutBufLen+=8;
				pOutBuf+=8;
			}
		}

		/*src_i指向src_buf下一个位置*/

		while(nInBufLen)
		{
			if (src_i<8)
			{
				src_buf[src_i++]=*(pInBuf++);
				nInBufLen--;
			}

			if (src_i==8)
			{
				/*src_i==8*/
				
				for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
					src_buf[j]^=iv_crypt[j];
				/*pOutBuffer、pInBuffer均为8byte, &m_arKey为16byte*/
				CryptFunc::TeaEncryptECB(src_buf, m_arKey, pOutBuf);

				for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
					pOutBuf[j]^=iv_plain[j];

				/*保存当前的iv_plain*/
				for (j=0;j<8;j++)
					iv_plain[j]=src_buf[j];

				src_i=0;
				iv_crypt=pOutBuf;
				nOutBufLen+=8;
				pOutBuf+=8;
			}
		}

		/*src_i指向src_buf下一个位置*/

		for (i=1;i<=ZERO_LEN;)
		{
			if (src_i<8)
			{
				src_buf[src_i++]=0;
				i++; /*i inc in here*/
			}

			if (src_i==8)
			{
				/*src_i==8*/
				
				for (j=0;j<8;j++) /*加密前异或前8个byte的密文(iv_crypt指向的)*/
					src_buf[j]^=iv_crypt[j];
				/*pOutBuffer、pInBuffer均为8byte, &m_arKey为16byte*/
				CryptFunc::TeaEncryptECB(src_buf, m_arKey, pOutBuf);

				for (j=0;j<8;j++) /*加密后异或前8个byte的明文(iv_plain指向的)*/
					pOutBuf[j]^=iv_plain[j];

				/*保存当前的iv_plain*/
				for (j=0;j<8;j++)
					iv_plain[j]=src_buf[j];

				src_i=0;
				iv_crypt=pOutBuf;
				nOutBufLen+=8;
				pOutBuf+=8;
			}
		}
	}
	else if(m_nEncryptArith ==CRYPT_3)	
	{
		CryptFunc::oi_symmetry_encrypt2(pInBuf, nInBufLen, m_arKey, pOutBuf, &nOutBufLen);
		//qq_symmetry_encrypt3(pInBuf,nInBufLen,
		//	GetVersionM(), GetVersionS(), m_dwUin,m_arKey,pOutBuf,&nOutBufLen);

	}
	else
	{
		assert(0);
	}
	return ;
}


bool CCrypt::Decrypt(const unsigned char* pInBuf, int nInBufLen,unsigned char* pOutBuf, int& nOutBufLen)
{
	if(m_nDecryptArith == CRYPT_2)
	{
		int nPadLen, nPlainLen;
		unsigned char dest_buf[8], zero_buf[8];
		const unsigned char *iv_pre_crypt, *iv_cur_crypt;
		int dest_i, i, j;
		const unsigned char *pInBufBoundary;
		int nBufPos;
		nBufPos = 0;
		
		if ((nInBufLen%8) || (nInBufLen<16)) 
			return false;

		CryptFunc::TeaDecryptECB(pInBuf, m_arKey, dest_buf);

		nPadLen = dest_buf[0] & 0x7/*只要最低三位*/;

		/*密文格式:PadLen(1QBYTE)+Padding(var,0-7QBYTE)+Salt(2QBYTE)+Body(var QBYTE)+Zero(7QBYTE)*/
		i = nInBufLen-1/*PadLen(1QBYTE)*/-nPadLen-SALT_LEN-ZERO_LEN; /*明文长度*/
		if ((nOutBufLen<i) || (i<0)) 
			return false;
		nOutBufLen = i;
		
		pInBufBoundary = pInBuf + nInBufLen; /*输入缓冲区的边界，下面不能pInBuf>=pInBufBoundary*/

		
		for ( i=0; i<8; i++)
			zero_buf[i] = 0;

		iv_pre_crypt = zero_buf;
		iv_cur_crypt = pInBuf; /*init iv*/

		pInBuf += 8;
		nBufPos += 8;

		dest_i=1; /*dest_i指向dest_buf下一个位置*/


		/*把Padding滤掉*/
		dest_i+=nPadLen;

		/*dest_i must <=8*/

		/*把Salt滤掉*/
		for (i=1; i<=SALT_LEN;)
		{
			if (dest_i<8)
			{
				dest_i++;
				i++;
			}
			else if (dest_i==8)
			{
				/*解开一个新的加密块*/

				/*改变前一个加密块的指针*/
				iv_pre_crypt = iv_cur_crypt;
				iv_cur_crypt = pInBuf; 

				/*异或前一块明文(在dest_buf[]中)*/
				for (j=0; j<8; j++)
				{
					if( (nBufPos + j) >= nInBufLen)
						return false;
					dest_buf[j]^=pInBuf[j];
				}

				/*dest_i==8*/
				CryptFunc::TeaDecryptECB(dest_buf, m_arKey, dest_buf);

				/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

				
				pInBuf += 8;
				nBufPos += 8;
		
				dest_i=0; /*dest_i指向dest_buf下一个位置*/
			}
		}

		/*还原明文*/

		nPlainLen=nOutBufLen;
		while (nPlainLen)
		{
			if (dest_i<8)
			{
				*(pOutBuf++)=dest_buf[dest_i]^iv_pre_crypt[dest_i];
				dest_i++;
				nPlainLen--;
			}
			else if (dest_i==8)
			{
				/*dest_i==8*/

				/*改变前一个加密块的指针*/
				iv_pre_crypt = iv_cur_crypt;
				iv_cur_crypt = pInBuf; 

				/*解开一个新的加密块*/

				/*异或前一块明文(在dest_buf[]中)*/
				for (j=0; j<8; j++)
				{
					if( (nBufPos + j) >= nInBufLen)
						return false;
					dest_buf[j]^=pInBuf[j];
				}

				CryptFunc::TeaDecryptECB(dest_buf, m_arKey, dest_buf);

				/*在取出的时候才异或前一块密文(iv_pre_crypt)*/
			
				
				pInBuf += 8;
				nBufPos += 8;
		
				dest_i=0; /*dest_i指向dest_buf下一个位置*/
			}
		}

		/*校验Zero*/
		for (i=1;i<=ZERO_LEN;)
		{
			if (dest_i<8)
			{
				if(dest_buf[dest_i]^iv_pre_crypt[dest_i]) return false;
				dest_i++;
				i++;
			}
			else if (dest_i==8)
			{
				/*改变前一个加密块的指针*/
				iv_pre_crypt = iv_cur_crypt;
				iv_cur_crypt = pInBuf; 

				/*解开一个新的加密块*/

				/*异或前一块明文(在dest_buf[]中)*/
				for (j=0; j<8; j++)
				{
					if( (nBufPos + j) >= nInBufLen)
						return false;
					dest_buf[j]^=pInBuf[j];
				}

				CryptFunc::TeaDecryptECB(dest_buf, m_arKey, dest_buf);

				/*在取出的时候才异或前一块密文(iv_pre_crypt)*/

				
				pInBuf += 8;
				nBufPos += 8;
				dest_i=0; /*dest_i指向dest_buf下一个位置*/
			}
		
		}

		return true;
	}
	else if(m_nDecryptArith ==CRYPT_3)
	{
		return CryptFunc::oi_symmetry_decrypt2(pInBuf, nInBufLen, m_arKey, pOutBuf, &nOutBufLen);
		//return qq_symmetry_decrypt3(pInBuf,nInBufLen,
		//	GetVersionM(), GetVersionS(),m_dwUin,m_arKey,pOutBuf,&nOutBufLen);
	}
	return false;
}

int CCrypt::FindEncryptSize(int nLen)
{
	int nPadSaltBodyZeroLen/*PadLen(1byte)+Salt+Body+Zero的长度*/;
	int nPadlen;

	assert(m_nEncryptArith == CRYPT_2 || m_nEncryptArith == CRYPT_3);
	if(m_nEncryptArith == CRYPT_2)
	{
	
		/*根据Body长度计算PadLen,最小必需长度必需为8byte的整数倍*/
		nPadSaltBodyZeroLen = nLen/*Body长度*/+1+SALT_LEN+ZERO_LEN/*PadLen(1byte)+Salt(2byte)+Zero(7byte)*/;
		if((nPadlen=nPadSaltBodyZeroLen%8) != 0) /*len=nSaltBodyZeroLen%8*/
		{
			/*模8余0需补0,余1补7,余2补6,...,余7补1*/
			nPadlen=8-nPadlen;
		}
	}
	else if(m_nEncryptArith ==CRYPT_3)
	{
		return CryptFunc::oi_symmetry_encrypt2_len(nLen);

	}
	else
	{
		assert(0);
		return 0;
	}
	return nPadSaltBodyZeroLen+nPadlen;
}

void CCrypt::Md5Hash(unsigned char *outBuffer, const unsigned char *inBuffer, int length)
{
	MD5_CTX *md5Info, md5InfoBuffer;
	md5Info = &md5InfoBuffer;
	
	MD5_Init( md5Info );
	MD5_Update( md5Info, inBuffer, length );
	MD5_Final( outBuffer, md5Info );
}

void CCrypt::SetKey(unsigned char* pKey,int nLen)
{
	assert(nLen =16);
	if(nLen==SESSION_KEY_SIZE)
		memcpy(m_arKey,pKey,SESSION_KEY_SIZE);
}

void CCrypt::SetArith(unsigned char nEncrypt, unsigned char nDecrypt)
{
	m_nEncryptArith = nEncrypt;
	m_nDecryptArith = nDecrypt;
}

unsigned char CCrypt::GetVersionM()
{
	return 0x04;
}

unsigned char CCrypt::GetVersionS()
{
	return 0x00;
}

void CCrypt::SetUin(uint32_t dwUin)
{
	m_dwUin = dwUin;
}


