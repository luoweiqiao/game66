/*
 * streampacket.h
 *
 *  Created on: 2011-12-7
 *      Author: toney
 */

#ifndef STREAMPACKET_H_
#define STREAMPACKET_H_
#include "packet/istreampacket.h"
#include "packet/ostreampacket.h"
class CStreamPacket
{
public:
    CStreamPacket(size_t uiOStreamSize):m_oOstreampacket(uiOStreamSize){}
    CStreamPacket():m_oOstreampacket(64*1024){}
    svrlib::CIStreamPacket& GetIstreamPacket()
    {
        return m_oIstreampacket;
    }

    const svrlib::CIStreamPacket& GetIstreamPacket() const
    {
        return m_oIstreampacket;
    }

    svrlib::COStreamPacket& GetOstreamPacket()
    {
        return m_oOstreampacket;
    }

    bool Unpack(const uint8_t * pData, size_t uiLen)
    {
        return m_oIstreampacket.Unpack(pData, uiLen);
    }

    bool Pack(uint8_t *pBuf, size_t& uiBufferLen) const
    {
        return m_oOstreampacket.Pack(pBuf, uiBufferLen);
    }

private:
    svrlib::CIStreamPacket m_oIstreampacket;
    svrlib::COStreamPacket m_oOstreampacket;
};

#endif /* STREAMPACKET_H_ */
