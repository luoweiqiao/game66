/*
 * lruvariablelengthhashtable.h
 *
 *  Created on: 2012-2-15
 *      Author: toney
 */

#ifndef LRUVARIABLELENGTHHASHTABLE_H_
#define LRUVARIABLELENGTHHASHTABLE_H_

/*
 * VariableLengthHashTable.h
 *
 *  Created on: 2012-2-8
 *      Author: toney
 */
#include "fundamental/common.h"
#include "fundamental/hashtable.h"
#include "utility/ipcutility.h"
namespace svrlib
{
template<typename KEY>
struct CLRUVariableLengthHashItem
{
public:
    size_t m_prevLRUOffset;
    size_t m_nextLRUOffset;

    //如果在free链表中，指向下一个同长度的free结点，如果不free，则指向同一Hash值的下一节点。
    size_t m_nextOffset;

    KEY m_oKey;
    uint32_t m_uiValueSize;
    uint8_t m_oValue[0];
};

struct LRUSlab
{
    size_t m_LRUHead; //本长度LRU链表头指针
    size_t m_LRUTail; //本长度LRU链表尾指针
    size_t m_free;    //本长度空闲链表指针
};

template<typename KEY>
class CLRUVariableLengthHashTable
{
public:

    /**
     * 创建在共享内存中的可变长HashTable,
     * @param iShmKey 共享内存的key,此共享内存会被重用(已创建）或创建(未创建)
     * @param uiMaxElementNumber 最大元素个数, 不能低于500,否则会抛出异常。
     * @param uiMemorySize 共享内存大小
     * @param uiMinValueLength value的最小长度
     * @param uiMaxValueLength value的最大长度
     */
    CLRUVariableLengthHashTable(int iShmKey, uint32_t uiMaxElementNumber,
            size_t uiMemorySize, uint32_t uiMinValueLength,
            uint32_t uiMaxValueLength) /*throw (const char*)*/ :
        m_oShm(iShmKey, uiMemorySize)
    {
        if (m_oShm.Get() == NULL)
        {
            throw "Initialize shared memory fails!";
        }
        if (uiMaxElementNumber < 500)
        {
            throw "the max element number is less than 500!";
        }
        m_pMomoryBegin = m_oShm.Get();
        m_uiMaxElementNumber = GetMaxPrime(uiMaxElementNumber);
        m_uiMemorySize = uiMemorySize;
        uint32_t uiMinElementLength = GetAlignedSize(
                uiMinValueLength + sizeof(CLRUVariableLengthHashItem<KEY> ));
        uint32_t uiMaxElementLength = GetAlignedSize(
                uiMaxValueLength + sizeof(CLRUVariableLengthHashItem<KEY> ));
        m_minPowerNumber = GetLeastPowerNumber(uiMinElementLength);
        m_maxPowerNumber = GetMostPowerNumber(uiMaxElementLength);
        /**
         * 内存布局：
         * 1.hash表数组
         * 2.slab数组
         * 3.是否初始化过
         * 4.空闲大块的大小
         * 5.指向空闲大块开始位置的指针
         *
         * 为简化操作，一律8字节对齐
         */
        size_t hashItemOffset = 0;
        size_t slabAreaOffset = GetAlignedSize(
                m_hashItemOffset + sizeof(size_t) * m_uiMaxElementNumber);
        size_t bInitializedOffset = GetAlignedSize(
                m_freeAreaOffset + sizeof(LRUSlab) * (m_maxPowerNumber
                        - m_minPowerNumber + 1));
        size_t uiFreeBlockSizeOffset = GetAlignedSize(
                m_bInitializedOffset + sizeof(bool));
        size_t freeBlockBeginOffset = GetAlignedSize(
                m_uiFreeBlockSizeOffset + sizeof(size_t));

        m_pHash = GetPointer<size_t>(hashItemOffset);
        m_pSlabArea = GetPointer<LRUSlab>(slabAreaOffset);

        bool *pInitialized = GetPointer<bool> (m_bInitializedOffset);

        m_puiFreeBlockSize = GetPointer<size_t>(uiFreeBlockSizeOffset);
        m_pFreeBlockBegin = GetPointer<size_t>(freeBlockBeginOffset);
        if (!*pInitialized)
        {
            *pInitialized = true;
            *m_pFreeBlockBegin = GetAlignedSize(
                    uiFreeBlockSizeOffset + sizeof(size_t));
            *m_puiFreeBlockSize = (uiMemorySize - *m_pFreeBlockBegin)
                    / BLOCK_UNIT_SIZE * BLOCK_UNIT_SIZE; //按1M为单位,简化后续分配操作
        }
    }

    Insert_Result Insert(const KEY & oKey, const uint8_t *m_pValue,
            size_t uiValueLength)
    {

        uint32_t uiPowerNumber = GetMostPowerNumber(
                GetAlignedElementSize(uiValueLength));
        if (uiPowerNumber < m_minPowerNumber || uiPowerNumber
                > m_maxPowerNumber)
        {
            return INSERT_FAIL;
        }

        uint32_t uiHash = GetHashVal(oKey) % m_uiMaxElementNumber;
        if (DoDelete(oKey, true, m_pValue, uiValueLength, uiPowerNumber, uiHash))
        {
            return INSERT_SUCCESS;
        }

        size_t uiOffset = AllocateSpace(uiPowerNumber);
        if (uiOffset == 0)
        {
            return INSERT_FAIL;
        }
        CLRUVariableLengthHashItem<KEY> *pItem = GetPointer<
                CLRUVariableLengthHashItem<KEY> > (uiOffset);
        pItem->m_oKey = oKey;
        pItem->m_uiValueSize = uiValueLength;
        memcpy(pItem->m_oValue, m_pValue, uiValueLength);

        InsertIntoHash(uiOffset, uiHash);

        if (double(*m_puiFreeBlockSize) / double(m_uiMemorySize) < 0.2)
        {
            return INSERT_SUCCESS_WILL_FULL;
        }
        return INSERT_SUCCESS;
    }

    //查找
    const CLRUVariableLengthHashItem<KEY>* Get(const KEY & oKey)
    {
        size_t *phashItem = m_pHash + GetHashVal(oKey) % m_uiMaxElementNumber;
        size_t uiOffset = *phashItem;
        if (uiOffset == 0)
        {
            return NULL;
        }
        CLRUVariableLengthHashItem<KEY> *item = NULL;
        while (uiOffset != 0)
        {
            item = GetPointer<CLRUVariableLengthHashItem<KEY> > (uiOffset);
            if (item->m_oKey == oKey)
            {
                break;
            }
            uiOffset = item->m_nextOffset;
        }

        if (uiOffset == 0)
        {
            return NULL;
        }

        return item;
    }

    //删除
    bool Delete(const KEY& oKey)
    {
        return DoDelete(oKey, false, NULL, 0, 0, 0);
    }

    ~CVariableLengthHashTable()
    {
    }

private:
    //删除或者改变，参数bIsChange表示是否需要改变
    bool DoDelete(const KEY& oKey, bool bIsChange, const uint8_t *pValue,
            size_t uiValueLength, uint32_t uiWantedPowerNumber, uint32_t uiHash)
    {
        if (!bIsChange)
        {
            uiHash = GetHashVal(oKey) % m_uiMaxElementNumber;
        }

        size_t uiOffset = m_pHash[uiHash];
        if (uiOffset == 0)
        {
            return false;
        }

        //链表不为空
        CLRUVariableLengthHashItem<KEY> *pFirstItem = GetPointer<
                CLRUVariableLengthHashItem<KEY> > (uiOffset);

        //是链表首结点
        if (pFirstItem->m_oKey == oKey)
        {
            if (bIsChange)
            {
                if (ChangeValue(pFirstItem, pValue, uiValueLength,
                        uiWantedPowerNumber))
                {
                    return true;
                }

            }
            m_pHash[uiHash] = pFirstItem->m_nextOffset;
            FreeSpace(uiOffset);
            return !bIsChange;
        }

        CLRUVariableLengthHashItem<KEY> *preItem = pFirstItem;
        uiOffset = preItem->m_nextOffset;
        CLRUVariableLengthHashItem<KEY> *selfItem = NULL;
        while (uiOffset != 0)
        {
            selfItem = GetPointer<CLRUVariableLengthHashItem<KEY> > (uiOffset);
            if (selfItem->m_oKey == oKey)
            {
                break;
            }
            preItem = selfItem;
            uiOffset = preItem->m_nextOffset;
        }
        if (uiOffset == 0)
        {
            return false;
        }

        //找到了
        if (bIsChange)
        {
            if (ChangeValue(selfItem, pValue, uiValueLength,
                    uiWantedPowerNumber))
            {
                return true;
            }
        }

        size_t uiNextOffset = selfItem->m_nextOffset;
        FreeSpace(preItem->m_nextOffset);
        preItem->m_nextOffset = uiNextOffset;
        return !bIsChange;
    }

    bool ChangeValue(CLRUVariableLengthHashItem<KEY> *pElement, const uint8_t* pValue,
            size_t uiValueLength, uint32_t uiWantedPowerNumber)
    {
        size_t alignedSize = GetAlignedElementSize(pElement->m_uiValueSize);
        uint32_t uiPowerNumber = GetMostPowerNumber(alignedSize);
        if (uiPowerNumber == uiWantedPowerNumber)
        {
            memcpy(&pElement->m_oValue, pValue, uiValueLength);
            pElement->m_uiValueSize = uiValueLength;
            return true;
        }
        return false;
    }

    void InsertIntoHash(size_t uiOffset, uint32_t uiHash)
    {
        size_t *phashItem = GetPointer<size_t> ((m_hashItemOffset));
        pItem->m_nextOffset = phashItem[uiHash];
        phashItem[uiHash] = uiOffset;
    }

    size_t AllocateSpace(size_t uiPowerNumber)
    {
        size_t uiOffSet = AllocateFromSlab(uiPowerNumber);
        if (uiOffSet == 0)
        {
            size_t uiBlockBeginOffSet = AllocateFromBlock();
            if (uiBlockBeginOffSet == 0)
            {
                return 0;
            }

            InsertIntoSlab(uiBlockBeginOffSet);
        }
        return AllocateFromSlab(uiPowerNumber);
    }

    size_t FreeSpace(size_t uiPowerNumber, size_t uiOffset)
    {
        CLRUVariableLengthHashItem<KEY> *item = GetPointer<
                CLRUVariableLengthHashItem<KEY> > (uiOffset);
        size_t uiValueLength = GetAlignedSize(
                item->m_uiValueSize + sizeof(CLRUVariableLengthHashItem<KEY> ));
        uint32_t uiPowerNumber = GetMostPowerNumber(uiValueLength);
        LRUSlab *pSlabArea = m_pSlabArea + (uiPowerNumber - m_minPowerNumber);
        item->m_nextOffset = pSlabArea->m_free;
        pSlabArea->m_free = uiOffset;
    }

    size_t AllocateFromBlock()
    {

        if (*m_puiFreeBlockSize == 0)
        {
            return 0;
        }
        //分配1M空间
        *m_puiFreeBlockSize -= BLOCK_UNIT_SIZE;
        size_t oldFreeBlockOffset = *m_pFreeBlockBegin;
        *m_pFreeBlockBegin +=  BLOCK_UNIT_SIZE;
        return oldFreeBlockOffset;
    }

    void InsertIntoSlab(size_t uiPowerNumber, size_t uiOffset)
    {
        size_t uiItemSize = 1 << uiPowerNumber;

        //将分配的1M空间切割成等大小的结点,加入空闲链表
        CLRUVariableLengthHashItem<KEY> *pItem = NULL;
        for (size_t i = 0; i < BLOCK_UNIT_SIZE - uiItemSize; i = i + uiItemSize)
        {
            pItem = GetPointer<CLRUVariableLengthHashItem<KEY> > (i);
            pItem->m_nextOffset = i + uiItemSize;
        }

        //尾结点没有后续结点
        LRUSlab *pSlabArea = m_pSlabArea + (uiPowerNumber - m_minPowerNumber);
        pItem = GetPointer<CLRUVariableLengthHashItem<KEY> > (
                uiOffset + BLOCK_UNIT_SIZE - uiItemSize);
        pItem->m_nextOffset = *pSlabArea->m_free;
        pSlabArea->m_free = uiOffset;
    }

    size_t AllocateFromSlab(size_t uiPowerNumber)
    {
        LRUSlab *pSlabArea =  m_pSlabArea + (uiPowerNumber - m_minPowerNumber);
        if (pSlabArea->m_free == 0)
        {
            return 0;
        }

        size_t offset = pSlabArea->m_free;
        CLRUVariableLengthHashItem<KEY> *pItem = GetPointer<
                CLRUVariableLengthHashItem<KEY> > (offset);
        pSlabArea->m_free = pItem->m_nextOffset;
        return offset;

    }

    //将offset转化成相应类型的指针
    template<typename type> type *GetPointer(size_t offset)
    {
        return (type*) ((m_pMomoryBegin + offset));
    }

    //2的几次方小于等于uiValue? 比如 uiValue 为15, 返回3,因为2^3<15 且2^4>15.
    uint32_t GetLeastPowerNumber(size_t uiValue)
    {
        size_t number = 1;
        uint32_t powerNumber = 0;
        while (number <= uiValue)
        {
            number = number << 1;
            powerNumber++;
        }
        powerNumber = powerNumber - 1;
        return powerNumber;
    }

    //2的几次方大于等于uiValue? 比如 uiValue 为15, 返回4,因为2^3<15 且2^4>15.
    uint32_t GetMostPowerNumber(size_t uiValueLength)
    {
        size_t number = 1;
        uint32_t powerNumber = 0;
        while (number < uiValueLength)
        {
            number = number << 1;
            powerNumber++;
        }
        return powerNumber;
    }

    uint32_t GetMaxPrime(uint32_t uiMaxElementNumber)
    {
        for (uint32_t iCurrent = uiMaxElementNumber; iCurrent > 0; iCurrent--)
        {
            if (IsPrime(iCurrent))
            {

                return iCurrent;
            }
        }
        return 0;

    }

    static size_t GetAlignedSize(size_t size)
    {
        return (size + ALIGN - 1) / ALIGN * ALIGN;
    }

    static size_t GetAlignedElementSize(size_t valueLength)
    {
        GetAlignedSize(valueLength + sizeof(CLRUVariableLengthHashItem<KEY> ));
    }

    enum
    {
        ALIGN = 8
    };
    //所有内存按照8字节对齐
    enum
    {
        BLOCK_UNIT_SIZE = 1024 * 1024
    };
    // 块以 1M为单位
    CShm m_oShm; //共享内存

    uint32_t m_uiMemorySize; //全部内存大小
    uint32_t m_uiMaxElementNumber;//最大元素个数

    //以幂为单位的元素最小长度，如果元素最小长度为8，则为3.
    uint32_t m_minPowerNumber;

    //以幂为单位的元素最大长度，如果元素最小长度为64，则为6.
    uint32_t m_maxPowerNumber;

    //内存块起始地址,由于在共享内存中，所有内存操作只能通过起始地址和偏移量操作
    uint8_t *m_pMomoryBegin;

    /**
     * 内存分布：
     * 1.hash表数组
     * 2.free链表数组
     * 3.是否初始化过
     * 4.空闲大块的大小
     * 5.指向空闲大块开始位置的指针(用偏移量表示的指针)
     *
     * 为简化操作，一律8字节对齐
     */
    size_t* m_pHash;
    LRUSlab* m_pSlabArea;

    size_t* m_puiFreeBlockSize;
    size_t* m_pFreeBlockBegin;
};
}
#endif /* LRUVARIABLELENGTHHASHTABLE_H_ */
