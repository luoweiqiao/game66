/*----------------- hmac_sha1.h 
*

*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

/*************************************************************/
namespace hmac_sha1
{
	unsigned int	hmac_sha1(const char*key,unsigned int key_length,const char*data,unsigned int data_length,char*output,unsigned int out_lentth);
};

#endif // __HMAC_SHA1_H__

