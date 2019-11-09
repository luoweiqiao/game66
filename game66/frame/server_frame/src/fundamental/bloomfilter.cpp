
#include "fundamental/bloomfilter.h"
#include "randomgenerator.h"
using namespace svrlib;
using namespace std;


static const uint8_t bit_mask[BITS_PER_CHAR] = {
                                                       0x01,  //00000001
                                                       0x02,  //00000010
                                                       0x04,  //00000100
                                                       0x08,  //00001000
                                                       0x10,  //00010000
                                                       0x20,  //00100000
                                                       0x40,  //01000000
                                                       0x80   //10000000
                                                     };



CBloomFilter::CBloomFilter(const uint64_t predicted_element_count,
                const double false_positive_probability,
                unsigned int shared_mem_Key)
    : m_u64PredictedCount(predicted_element_count),
      m_dFalsePositiveProbability(false_positive_probability)

    {
        FindOptimalParameters();

        m_ShmSize = m_u64BitSize + sizeof(m_pHead);
        m_pShm = new CShm(shared_mem_Key, m_ShmSize);
        if (m_pShm->Get() == NULL)
        {
            throw "Init Shm Bit Table Failed";
        }

        m_pHead     = (SBlommFilterHead*)(m_pShm->Get());
        m_pBitTable = (uint8_t*)(m_pHead + 1);
    }


    void CBloomFilter::Insert(const uint8_t* pKey, size_t length, bool *pbIsNewlyInserted)
    {
        bool bIsNewlyInserted = false;
        uint64_t bit_index = 0;
        uint64_t bit = 0;
        UInt64RandomGenerator MTRng(pKey, length);
        for (std::size_t i = 0; i < m_u64HashCount; ++i)
        {
            ComputeIndices(MTRng.GetRandomNumber(), bit_index, bit);
            if ((m_pBitTable[bit_index / BITS_PER_CHAR] & bit_mask[bit])
                    != bit_mask[bit])
            {
                bIsNewlyInserted = true;
            }
            m_pBitTable[bit_index / BITS_PER_CHAR] |= bit_mask[bit];
        }
        m_pHead->u64InsertCount++;
        if (bIsNewlyInserted)
            m_pHead->u64ElementCount++;
        if(pbIsNewlyInserted != NULL)
        {
            *pbIsNewlyInserted = bIsNewlyInserted;
        }
    }

    bool CBloomFilter::IsContain(const uint8_t* key_begin, size_t length) const
    {
        uint64_t bit_index = 0;
        uint64_t bit = 0;
        UInt64RandomGenerator MTRng(key_begin, length);
        for (std::size_t i = 0; i < m_u64HashCount; ++i)
        {
            ComputeIndices(MTRng.GetRandomNumber(), bit_index, bit);
            if ((m_pBitTable[bit_index / BITS_PER_CHAR] & bit_mask[bit])
                    != bit_mask[bit])
            {
                return false;
            }
        }
        return true;
    }



    void CBloomFilter::FindOptimalParameters()
    {
        /*
          Note:
          The following will attempt to find the number of hash functions
          and minimum amount of storage bits required to construct a bloom
          filter consistent with the user defined false positive probability
          and estimated element insertion count.
        */
        double min_m = std::numeric_limits<double>::infinity();
        double min_k = 0.0;
        double curr_m = 0.0;
        for (double k = 0.0; k < 1000.0; ++k)
        {
            if ((curr_m = ((- k * m_u64PredictedCount) / std::log(1.0 - std::pow(m_dFalsePositiveProbability, 1.0 / k)))) < min_m)
            {
                min_m = curr_m;
                min_k = k;
            }
        }

        m_u64HashCount = static_cast<uint64_t>(min_k);
        m_u64BitCount = static_cast<uint64_t>(min_m);
        m_u64BitCount += (((m_u64BitCount % BITS_PER_CHAR) != 0) ? (BITS_PER_CHAR - (m_u64BitCount % BITS_PER_CHAR)) : 0);
        m_u64BitSize = m_u64BitCount / BITS_PER_CHAR;
    }

