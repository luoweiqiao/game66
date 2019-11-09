/*
 * linkcache.cpp
 *
 *  Created on: 2012-2-21
 *      Author: toney
 */
#include "fundamental/linkcache.h"
#include <iostream>
#include <algorithm>
#include <string.h>

using namespace svrlib;

CLinkCache::CLinkCache(uint8_t* pBaseAddress, size_t uiMemorySize,
        size_t uiValueUnitSize) :
    m_pBaseAddress(pBaseAddress)
{

    /**
     * 内存对齐规则：
     * 1.如需要将指针转化为数据长度为1的类型,如uint8_t*等，则不需对齐。
     * 2.其它类型如size_t* 一律8字节对齐。
     */
    size_t uiUnitSize = GetAlignedSize(sizeof(CLinkItem) + uiValueUnitSize);
    //value为unit8_t类型，不需对齐
    m_unitValueSize = uiUnitSize - sizeof(CLinkItem);

    size_t uiMemoryOffset = 0;

    /*
     * 内存布局：
     * 1.是否初始化过。
     * 2.free链表头结点地址。
     * 3.free链表中空闲结点数。
     */
    bool *pIsInitialized = GetPointer<bool> (uiMemoryOffset);
    uiMemoryOffset += GetAlignedSize(sizeof(bool));
    m_pFreeLinkHead = GetPointer<size_t> (uiMemoryOffset);
    uiMemoryOffset += GetAlignedSize(sizeof(size_t));
    m_pFreeUnitSize = GetPointer<size_t> (uiMemoryOffset);
    uiMemoryOffset += GetAlignedSize(sizeof(size_t));

    size_t uiFreeNodeBeginOffset = uiMemoryOffset;
    if (!*pIsInitialized)
    {
        *pIsInitialized = true;
        size_t uiPoolSize = (uiMemorySize - uiFreeNodeBeginOffset) / uiUnitSize
                * uiUnitSize;
        for (size_t uiOffset = 0; uiOffset < uiPoolSize - uiUnitSize; uiOffset
                += uiUnitSize)
        {

            size_t uiPosition = uiFreeNodeBeginOffset + uiOffset;
            CLinkItem *pItem = GetPointer<CLinkItem> (uiPosition);
            pItem->m_nextOffset = uiPosition + uiUnitSize;
        }

        //最后一个结点
        size_t position = uiPoolSize - uiUnitSize + uiFreeNodeBeginOffset;
        CLinkItem *pItem = GetPointer<CLinkItem> (position);
        pItem->m_nextOffset = 0;

        *m_pFreeLinkHead = uiFreeNodeBeginOffset;
        *m_pFreeUnitSize = uiPoolSize / uiUnitSize;
    }

}

CLinkItem* CLinkCache::Set(size_t uiOffset, const uint8_t *m_pValue,
        size_t uiValueLength)
{
    const size_t uiUnitNumber = GetUnitNumber(uiValueLength);
    for (size_t i = 0; i < uiUnitNumber - 1; i++)
    {
        CLinkItem* item = GetPointer<CLinkItem> (uiOffset);
        item->m_uiValueSize = m_unitValueSize;
        memcpy(&item->m_oValue, m_pValue, m_unitValueSize);
        uiOffset = item->m_nextOffset;
        m_pValue += m_unitValueSize;
        uiValueLength -= m_unitValueSize;
    }
    //最后一个结点
    CLinkItem* pItem = GetPointer<CLinkItem> (uiOffset);
    pItem->m_uiValueSize = uiValueLength;
    memcpy(&pItem->m_oValue, m_pValue, pItem->m_uiValueSize);
    return pItem;
}
size_t CLinkCache::Set(const uint8_t *m_pValue, size_t uiValueLength)
{
    size_t uiAllocatedOffset = AllocateSpace(uiValueLength);
    size_t uiOffset = uiAllocatedOffset;

    if (uiOffset == 0)
    {
        return 0;
    }
    CLinkItem* pItem = Set(uiOffset, m_pValue, uiValueLength);
    pItem->m_nextOffset = 0;
    return uiAllocatedOffset;
}

size_t CLinkCache::Set(const uint8_t*m_pValue1, size_t uiValueLength1,
        const uint8_t* m_pValue2, size_t uiValueLength2)
{
    size_t uiAllocatedOffset = AllocateSpace(uiValueLength1 + uiValueLength2);
    size_t uiOffset = uiAllocatedOffset;

    if (uiOffset == 0)
    {
        return 0;
    }

    CLinkItem* pItem = Set(uiOffset, m_pValue1, uiValueLength1);

    uint32_t uiRemainedFreeLength = m_unitValueSize - pItem->m_uiValueSize;

    uint32_t copyLength = ((size_t)uiRemainedFreeLength < uiValueLength2) ? uiRemainedFreeLength : uiValueLength2;

    memcpy(pItem->m_oValue + pItem->m_uiValueSize, m_pValue2, copyLength);
    pItem->m_uiValueSize += copyLength;
    m_pValue2 += copyLength;
    uiValueLength2 -= copyLength;

    if (uiValueLength2 > 0)
    {
        uiOffset = pItem->m_nextOffset;
        pItem = Set(uiOffset, m_pValue2, uiValueLength2);
    }
    //最后一个结点
    pItem->m_nextOffset = 0;
    return uiAllocatedOffset;
}

CVariableLengthHashTable_Get_Result CLinkCache::Get(size_t uiOffset,
        uint8_t* pValue, size_t& valueLength) const
{
    return Get(uiOffset, 0, pValue, valueLength);
}

CVariableLengthHashTable_Get_Result CLinkCache::Get(size_t uiOffset,
        size_t skipSpaces, uint8_t* pValue, size_t& valueLength) const
{
    size_t uiRemainedLength = valueLength;
    size_t uiRemainedskipSpaces = skipSpaces;
    size_t uiTotalLength = 0;
    while (uiOffset != 0)
    {
        CLinkItem* pItem = GetPointer<CLinkItem> (uiOffset);
        uiOffset = pItem->m_nextOffset;
        if (uiRemainedskipSpaces >= pItem->m_uiValueSize)
        {
            uiRemainedskipSpaces -= pItem->m_uiValueSize;
            continue;
        }

        if (uiRemainedLength < pItem->m_uiValueSize)
        {
            return BUFFER_LEGNTH_TOO_SMALL;
        }
        uint8_t* pSrc = pItem->m_oValue + uiRemainedskipSpaces;
        size_t copyLength = pItem->m_uiValueSize - uiRemainedskipSpaces;
        memcpy(pValue + uiTotalLength, pSrc, copyLength);
        uiTotalLength += copyLength;
        uiRemainedLength -= copyLength;
        uiRemainedskipSpaces = 0;
    }
    valueLength = uiTotalLength;
    return GET_SUCCESS;
}

void CLinkCache::FreeSpace(size_t uiOffset)
{
    CLinkItem* pItem = GetPointer<CLinkItem> (uiOffset);
    while (pItem->m_nextOffset != 0)
    {
        pItem = GetPointer<CLinkItem> (pItem->m_nextOffset);
    }
    pItem->m_nextOffset = *m_pFreeLinkHead;
    *m_pFreeLinkHead = uiOffset;
}

size_t CLinkCache::AllocateSpace(size_t uiValueLength)
{
    size_t uiUnitNumber = (uiValueLength + m_unitValueSize - 1)
            / m_unitValueSize;
    if (*m_pFreeUnitSize < uiUnitNumber)
    {
        return 0;
    }
    size_t uiHead = *m_pFreeLinkHead;
    size_t uiOffset = uiHead;
    for (size_t i = 0; i < uiUnitNumber; i++)
    {
        if (uiOffset == 0)
        {
            //链表已经混乱
            return 0;
        }
        CLinkItem* pItem = GetPointer<CLinkItem> (uiOffset);
        uiOffset = pItem->m_nextOffset;
    }
    *m_pFreeLinkHead = uiOffset;
    return uiHead;
}

