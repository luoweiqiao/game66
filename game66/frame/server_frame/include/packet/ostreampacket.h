#ifndef _OSTREAM_PACKET_H_
#define _OSTREAM_PACKET_H_

#include <string>
#include <vector>
#include "fundamental/common.h"
#include "framework/logger.h"

namespace svrlib
{
#define CHECK_CANWRITE(t) {\
							if(m_uiConsumedLength + sizeof(t) > m_oBuffer.size())\
							{\
								return *this;\
							}\
						 }
#define COPY_DATA(t) {memcpy(&m_oBuffer[m_uiConsumedLength], &t, sizeof(t));m_uiConsumedLength += sizeof(t) ;}

class COStreamPacket
{
	typedef std::vector<uint8_t> BUF ;
public:
	COStreamPacket(size_t uiMaxSize)
		: m_oBuffer(uiMaxSize), m_uiConsumedLength(0)
	{
	}
    
    void Clear()
    {
        m_uiConsumedLength = 0;
    }
	
	bool Pack(uint8_t *pBuf, size_t& uiBufferLen) const
	{
	  if(m_uiConsumedLength > uiBufferLen)
	  {
	      return false;
	  }

	  uiBufferLen = m_uiConsumedLength;
	  memcpy(pBuf, &m_oBuffer[0], m_uiConsumedLength);
      return true;
	}
	
	size_t GetLen() const
	{
		return m_uiConsumedLength ;
	}
	const uint8_t * GetData() const
	{
		return &m_oBuffer[0] ;
	}


	void Skip(size_t uiSkipLen) /*throw (const char*)*/
	{
		if (m_uiConsumedLength + uiSkipLen >  m_oBuffer.size())
		{
		    throw "invalid packet.";
		}
		m_uiConsumedLength += uiSkipLen ;
	}

	template <class T>
	COStreamPacket& operator << (T const& t) /*throw (const char*)*/
	{
		CHECK_CANWRITE(t)
		T oTmp(t) ;
		oTmp.ToNetOrder() ;
		COPY_DATA(oTmp)
		return *this ;
	}
	
	COStreamPacket& WriteFixLenData(const uint8_t * pData, size_t uiLen) /*throw (const char*)*/
	{
		if (m_uiConsumedLength + uiLen > m_oBuffer.size())
		{
			throw "invalid packet.";
		}
		memcpy(&m_oBuffer[m_uiConsumedLength], pData, uiLen) ;
		m_uiConsumedLength += uiLen ;
		return *this ;
	}
	
	COStreamPacket& operator << (char t)/*throw (const char*)*/
	{
		CHECK_CANWRITE(t) 
		COPY_DATA(t)
		return *this ;
	}
	COStreamPacket& operator <<(int8_t t) /*throw (const char*)*/
    {
	    return operator<<((char)t) ;
    }
	COStreamPacket& operator << (uint8_t t)/*throw (const char*)*/
	{
		return operator<<((char)t) ;
	}

	COStreamPacket& operator << (uint16_t t)/*throw (const char*)*/
	{
		CHECK_CANWRITE(t) 
		t = htons(t) ;
		COPY_DATA(t)
		return *this ;
	}
	COStreamPacket& operator << (int16_t t)/*throw (const char*)*/
	{
		return operator<<((unsigned short)t) ;
	}

	COStreamPacket& operator << (uint32_t t)/*throw (const char*)*/
	{
		CHECK_CANWRITE(t) 
		t = htonl(t) ;
		COPY_DATA(t)
		return *this ;
	}
	COStreamPacket& operator << (int32_t t)/*throw (const char*)*/
	{
		return operator<<((uint32_t)t) ;
	}
	
	COStreamPacket& operator <<(int64_t t)/*throw (const char*)*/
    {
        return operator<<((uint64_t) t);
    }
	
	COStreamPacket& operator << (uint64_t t)/*throw (const char*)*/
	{
		CHECK_CANWRITE(t)

		t = htonll(t);
		COPY_DATA(t)
		return *this ;
	}

	COStreamPacket& operator << (const std::string& t) /*throw (const char*)*/
	{
		if (m_uiConsumedLength + sizeof(uint16_t) + t.length() > m_oBuffer.size())
		{
			return *this ;
		}
		operator<<((uint16_t)t.length()) ;
		if (t.length() > 0)
		{
			memcpy(&m_oBuffer[m_uiConsumedLength], t.c_str(), t.length()) ;
			m_uiConsumedLength += t.length() ;
		}
		return *this ;
	}

	COStreamPacket& operator << (BUF const& t) /*throw (const char*)*/
	{
		if (m_uiConsumedLength + sizeof(uint16_t) + t.size() > m_oBuffer.size())
		{
			return *this ;
		}
		operator<<((uint16_t)t.size()) ;
		if (t.size() > 0)
		{
			memcpy(&m_oBuffer[m_uiConsumedLength], &t[0], t.size()) ;
			m_uiConsumedLength += t.size() ;
		}
		return *this ;
	}

private:
	BUF m_oBuffer;
	size_t m_uiConsumedLength ;
} ;
};
#endif

