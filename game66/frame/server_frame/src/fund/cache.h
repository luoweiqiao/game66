#ifndef __CACHE_H__
#define __CACHE_H__

#include "hashtable.h"

namespace fund
{
template<typename KEY, typename DATA>
class CCache
{
	struct CacheNode
	{
		DATA oData ;
		CacheNode * prec ;
		CacheNode * next ;
		CacheNode()
		{
			prec = this ;
			next = this ;
		}
		CacheNode(DATA const& data)
			: oData(data)
		{
			prec = this ;
			next = this ;
		}
	};


public:
	CCache(unsigned int capacity, unsigned int bucketCnt)
		: mHashTable(capacity, bucketCnt), mCacheSize(capacity)
	{
		mLruHead = new CacheNode ;
		mLruHead->prec = mLruHead ;
		mLruHead->next = mLruHead ;
		mEnumPtr = mLruHead ;
	}
	~CCache()
	{
		delete mLruHead;
	}
	
	inline DATA * Get(KEY const& oKey) {
		CacheNode * pNode = mHashTable.Get(oKey) ;
		if (pNode)
		{
			DelListNode(pNode) ;
			AddListNode(pNode) ;
			return &(pNode->oData) ;
		}
		return 0 ;
	}
	inline DATA * Add(KEY const& oKey, const DATA& data) {
		if (mHashTable.Size() >= mCacheSize)
		{
			DelListNode(mLruHead->prec);
			mHashTable.Del(mHashTable.GetKeyByValue(mLruHead->prec)) ;
//			mHashTable.Del(mLruHead->prec->oData.GetKey()) ;
		}
		CacheNode * pNode = mHashTable.Add(oKey, data) ;
		if (pNode)
		{
			AddListNode(pNode) ;
			return &(pNode->oData) ;
		}
		return 0 ;
	}
	bool Del(KEY const& oKey) 
	{
		CacheNode * pNode = mHashTable.Get(oKey) ;
		if(pNode)
			DelListNode(pNode);
		return mHashTable.Del(oKey) ;
	}
	unsigned int Size() const
	{
		return mHashTable.Size() ;
	}

	DATA * EnumNext(KEY& oKey) {
		if (Size() > 0) {
			mEnumPtr = mEnumPtr->next ;
			if (mEnumPtr == mLruHead)
			{
				mEnumPtr = mEnumPtr->next ;
			}
			if (mEnumPtr == mLruHead)
			{
				throw "some thing error in Cache EnumNext." ;
			}
			oKey = mHashTable.GetKeyByValue(mEnumPtr) ;
			return &(mEnumPtr->oData) ;
		}
		return 0 ;
	}
private:
	inline void DelListNode(CacheNode * pNode)
	{
		if (pNode == mEnumPtr)
		{
			mEnumPtr = mEnumPtr->next ;
		}
		pNode->prec->next = pNode->next ;
		pNode->next->prec = pNode->prec ;
	}
	inline void AddListNode(CacheNode * pNode)
	{
		pNode->prec = mLruHead ;
		pNode->next = mLruHead->next ;
		mLruHead->next->prec = pNode ;
		mLruHead->next = pNode ;
	}
private:
	CacheNode * mLruHead ;
	CacheNode * mEnumPtr ;
	HashTable<CacheNode, KEY> mHashTable ;
	unsigned int mCacheSize ;
};
} ;
#endif//__CACHE_H__


