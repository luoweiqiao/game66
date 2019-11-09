/*
 *********************************************************************
 *                                                                   *
 *                           Open Bloom Filter                       *
 *                                                                   *
 * Author: Arash Partow - 2000                                       *
 * URL: http://www.partow.net                                        *
 * URL: http://www.partow.net/programming/hashfunctions/index.html   *
 *                                                                   *
 * Copyright notice:                                                 *
 * Free use of the Open Bloom Filter Library is permitted under the  *
 * guidelines and in accordance with the most current version of the *
 * Common Public License.                                            *
 * http://www.opensource.org/licenses/cpl1.0.php                     *
 *                                                                   *
 *********************************************************************
*/
/*
  Note 1:
  If it can be guaranteed that bits_per_char will be of the form 2^n then
  the following optimization can be used:

  hash_table[bit_index >> n] |= bit_mask[bit_index & (bits_per_char - 1)];

  Note 2:
  For performance reasons where possible when allocating memory it should
  be aligned (aligned_alloc) according to the architecture being used.
*/
#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H
#include <algorithm>
#include <cmath>
#include <limits>
#include "fundamental/common.h"
#include "utility/ipcutility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace svrlib
{
struct SBlommFilterHead
{
	uint64_t u64InsertCount;
	uint64_t u64ElementCount;
	uint8_t  u8Reserverve[48];
};

#define BITS_PER_CHAR 0x08 // 8 bits in 1 char(unsigned)

class CBloomFilter
{
protected:
	typedef uint64_t bloom_type;

public:
	CBloomFilter(const uint64_t u64PredictedElementCount,
				const double falsePositiveProbability,
				unsigned int uiSharedMemKey);

	~CBloomFilter()
	{
		delete m_pShm;
	}

	void Insert(const uint8_t* pKey, size_t length, bool *pbIsNewlyInserted = NULL);

	bool IsContain(const uint8_t* pKey, size_t length) const;

	void clear()
	{
		memset(m_pShm->Get(), 0, m_ShmSize);
	}


	uint64_t GetSize() const
	{
		return m_u64BitCount;
	}

	uint64_t GetInsertCount() const
	{
		return m_pHead->u64InsertCount;
	}

	uint64_t GetElementCount() const
	{
		return m_pHead->u64ElementCount;
	}

	void PrintParameters() const
	{
		printf("m_u64HashCount=%10llu, m_u64BitCount=%10llu, m_u64BitSize=%10llu\n",
			static_cast<unsigned long long>(m_u64HashCount),
			static_cast<unsigned long long>(m_u64BitCount),
			static_cast<unsigned long long>(m_u64BitSize));
	}

private:
	void ComputeIndices(const bloom_type& hash, uint64_t& bit_index, uint64_t& bit) const
	{
		bit_index = hash % m_u64BitCount;
		bit = bit_index % BITS_PER_CHAR;
	}

	void FindOptimalParameters();

	CShm*           m_pShm;
	size_t                  m_ShmSize;
	SBlommFilterHead*       m_pHead;
	uint8_t*                m_pBitTable;
	uint64_t                m_u64BitCount;
	uint64_t                m_u64BitSize;

	uint64_t                m_u64HashCount;

	uint64_t                m_u64PredictedCount;
	uint64_t                m_u64RandomSeed;
	double                  m_dFalsePositiveProbability;
};
}
#endif //_BLOOM_FILTER_H__



