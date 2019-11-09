#if !defined(AFX_CRYPT_H__A05365F3_110F_4771_A093_37F147F3CB94__INCLUDED_)
#define AFX_CRYPT_H__A05365F3_110F_4771_A093_37F147F3CB94__INCLUDED_

#include "fundamental/common.h"

namespace svrlib
{
class CCrypt
{
public:
	enum
	{
		CRYPT_2 = 0,
		CRYPT_3 = 1,
		SESSION_KEY_SIZE = 16,
	};
	void SetArith(unsigned char nEncrypt = CRYPT_3, unsigned char nDecrypt = CRYPT_2);
	void SetKey(unsigned char* pKey,int nLen=16);
	static void Md5Hash(unsigned char *outBuffer, const unsigned char *inBuffer, int length);
	int FindEncryptSize(int nLen);
	void Encrypt(const unsigned char* pInBuf, int nInBufLen, unsigned char* pOutBuf, int& pOutBufLen);
	bool Decrypt(const unsigned char* pInBuf, int nInBufLen, unsigned char* pOutBuf, int& nOutBufLen);
	CCrypt(unsigned char* pKey,unsigned char nEncryptArith,unsigned char nDecryptArith);
	CCrypt();
	virtual ~CCrypt();
	void SetUin(uint32_t dwUin);
private:
	unsigned char GetVersionM();
	unsigned char GetVersionS();

protected:
	unsigned char	m_arKey[SESSION_KEY_SIZE];
	unsigned char	m_nEncryptArith;
	unsigned char	m_nDecryptArith;

private:
	uint32_t m_dwUin;
};
}
#endif


