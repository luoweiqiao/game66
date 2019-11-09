#pragma once
#include "CCStreamDecoder.h"

class InnerDecoder : public CCStreamDecoder
{
public:
	InnerDecoder(){};
	virtual ~InnerDecoder(){};

protected:

	virtual inline int GetHeadLen(const char * data, int len)
	{
		return sizeof(int);
	}

	virtual inline  int GetPacketLen(const char * data, int len)
	{
		int pkglen = ntohl(*((int*)data));

		if (pkglen<0 || pkglen>16 * 1024)
		{
			//log_error("%s||Invalid packet, pkglen:[%d]", __FUNCTION__, pkglen);
			return -1;
		}

		return pkglen;
	}

	virtual inline  bool CheckHead(const char * data, int len)
	{
		return true;
	}
};

