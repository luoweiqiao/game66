#pragma once
#include "CCStreamDecoder.h"

class MsgDecoder : public CCStreamDecoder
{
public:
    MsgDecoder(){};
    virtual ~MsgDecoder(){};

protected:

	virtual inline int GetHeadLen(const char * data, int len)
	{
		(void)len;
		return sizeof(TPkgHeader) ;
	}

	virtual inline  int GetPacketLen(const char * data, int len)
	{
		TPkgHeader *pHeader = (struct TPkgHeader*)data;
        //int pkglen = ntohl(pHeader->datalen) + 4;
        int pkglen = pHeader->datalen + 8;

		if(pkglen<0 || pkglen>16*1024)
		{
			//log_error("%s||Invalid packet, pkglen:[%d]", __FUNCTION__, pkglen);
			return -1;
		}

		if(len < pkglen)
		{
			return 0;
		}
		return pkglen;
	}

	virtual inline  bool CheckHead(const char * data, int len)
	{
        return true;
	} 
};



class PhpDecoder : public CCStreamDecoder
{
public:
    PhpDecoder(){};
    virtual ~PhpDecoder(){};

protected:

    virtual inline int GetHeadLen(const char * data, int len)
    {
        (void)len;
        return 4;
    }

    virtual inline  int GetPacketLen(const char * data, int len)
    {
        PhpHeader *pHeader = (struct PhpHeader*)data;
        int pkglen = pHeader->datalen + 4;

        if (pkglen<0 || pkglen>16 * 1024)
        {
            //log_error("%s||Invalid packet, pkglen:[%d]", __FUNCTION__, pkglen);
            return -1;
        }

        if (len < pkglen)
        {
            return 0;
        }
        return pkglen;
    }

    virtual inline  bool CheckHead(const char * data, int len)
    {
        return true;
    }
};
