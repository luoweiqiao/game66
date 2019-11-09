#ifndef _ISTREAM_PACKET_H_
#define _ISTREAM_PACKET_H_


#include "framework/logger.h"


#define CHECK_CANREAD(t) {\
							if(m_uiLen < sizeof(t))\
							{\
								return *this;\
							}\
						}
#define READ_DATA(t, type) {t = *((const type *)m_pData) ; m_pData += sizeof(t) ; m_uiLen -= sizeof(t) ;}

#include <string>
#include <vector>
#include "fundamental/common.h"

namespace svrlib
{
class CIStreamPacket
{
	typedef std::vector<unsigned char> BUF ;
public:
	CIStreamPacket()
		: m_pData(0), m_uiLen(0)
	{
	}
	
	bool Unpack(const uint8_t * pData, size_t uiLen)
	{
		m_pData = pData;

        RETURN_FALSE_IF_NULL(pData);

        m_uiLen = uiLen;
        return true;

	}

	size_t GetRemainedLength() const
	{
		return m_uiLen ;
	}


	const uint8_t* GetRemainedData() const
	{
		return m_pData ;
	}

	void Skip(size_t uiSkipLen) /*throw (char const *)*/
	{
		if (m_uiLen < uiSkipLen)
		{
			throw "invalid skip." ;
		}
		m_pData += uiSkipLen ;
		m_uiLen -= uiSkipLen ;
	}

	template <class T>
	CIStreamPacket& operator >>(T& t)  /*throw (char const *)*/
	{
		CHECK_CANREAD(t)
		READ_DATA(t, T)
		t.ToHostOrder() ;
		return *this ;
	}

	CIStreamPacket& operator >>(char& t)  /*throw (char const *)*/
	{
		CHECK_CANREAD(t)
		READ_DATA(t, char)
		return *this ;
	}
	CIStreamPacket& operator >>(int8_t& t) /*throw (char const *)*/
    {
	    return operator>>((char&)t);
    }
	CIStreamPacket& operator >>(uint8_t& t)  /*throw (char const *)*/
	{
		return operator>>((char&)t) ;
	}

	CIStreamPacket& operator >>(int16_t& t)  /*throw (char const *)*/
	{
		CHECK_CANREAD(t)
		READ_DATA(t, int16_t)
		t = ntohs(t) ;
		return *this;
	}
	CIStreamPacket& operator >>(uint16_t& t)  /*throw (char const *)*/
	{
		return operator>>((int16_t&)t) ;
	}
	
	CIStreamPacket& operator >>(int32_t& t)  /*throw (char const *)*/
	{
		CHECK_CANREAD(t)
		READ_DATA(t, int32_t)
		t = ntohl(t) ;
		return *this ;
	}
	CIStreamPacket& operator >>(uint32_t& t)  /*throw (char const *)*/
	{
		return operator>>((int32_t&)t) ;
	}

	CIStreamPacket& operator >>(int64_t& t) /*throw (char const *)*/
    {
        return operator>>((uint64_t&) t);
    }

	CIStreamPacket& operator >>(uint64_t& t)  /*throw (char const *)*/
	{
        CHECK_CANREAD(t)
        READ_DATA(t, uint64_t)
        t = ntohll(t);
        return *this ;
	}
	
	CIStreamPacket& operator >>(std::string& t)  /*throw (char const *)*/
	{
		uint16_t usLen = 0;
		operator>>(usLen) ;
		if (usLen > 0)
		{
			if (m_uiLen < usLen)
			{
				throw "invalid packet." ;
			}
			t.assign((char const *)m_pData, usLen) ;
			m_pData += usLen ;
			m_uiLen -= usLen ;
		}
		else
		{
			t = "" ;
		}
		return *this ;
	}
	
	CIStreamPacket& operator >>(BUF& t)  /*throw (char const *)*/
	{
	    uint16_t usLen = 0;
		operator>>(usLen) ;
		if (usLen > 0)
		{
			if (m_uiLen < usLen)
			{
				throw "invalid packet." ;
			}
			t.resize(usLen) ;
			memcpy(&t[0], m_pData, usLen) ;
			m_pData += usLen ;
			m_uiLen -= usLen ;
		}
		else
		{
			t.clear() ;
		}
		return *this ;
	}
	
	CIStreamPacket& GetFixLenData(uint8_t * pData, size_t uiLen) /*throw (char const *)*/
	{
		if (m_uiLen < uiLen)
		{
			throw "invalid packet." ;
		}
		memcpy(pData, m_pData, uiLen) ;
		m_pData += uiLen ;
		m_uiLen -= uiLen ;
		return *this ;
	}
	
private:
	const uint8_t * m_pData ;
	size_t m_uiLen ;
} ;
}
#endif

