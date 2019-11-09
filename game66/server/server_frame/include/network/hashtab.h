#ifndef _HASHTAB_HPP_
#define _HASHTAB_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "idxobj.h"

#define USEEXISTED 0
#define NEWCREATED 1


typedef struct
{
	int m_iFirstObjIdx;
} THashItem;

template<int iSize>
class CHashTab
{
public:
	CHashTab();
	CHashTab(CObjSeg *pObjMng, short nHashNext);
	virtual ~CHashTab();
	
	virtual int HashKeyToIdx(const void *pvKey, int iKeyLength);

	int Initialize(CObjSeg *pObjMng, short nHashNext);
	int Resume(CObjSeg *pObjMng);

	THashItem& operator []( int iIdx );
	
	int Initialize();
	
	CObj* CreateObjectByKey( const void *pvKey, int iKeyLength, int& iErrNo, int *pNew = NULL );	
	CObj* GetObjectByKey( const void *pvKey, int iKeyLength );
	int DeleteObjectByKey( const void *pvKey, int iKeyLength );
	
	int m_iHashGetNumber;
	int m_iHashSearchNumber;
	int m_iHashNullNumber;

private:
	short		m_nHashNext;
	CObjSeg*	m_pObjMng;
	THashItem m_astHashItems[iSize];
};

template<int iSize>
CHashTab<iSize>::CHashTab()
{
	m_pObjMng = NULL;
	
	/*
	m_nHashNext = -1;

	if( !CObj::nCreateMode )
	{
		m_nHashNext = -1;
		Initialize();
	}
	*/
}

template<int iSize>
CHashTab<iSize>::CHashTab(CObjSeg *pObjMng, short nHashNext) : m_pObjMng(pObjMng), m_nHashNext(nHashNext)
{
	if( !CObj::nCreateMode )
	{
		Initialize();
	}
}

template<int iSize>
CHashTab<iSize>::~CHashTab()
{
}

template<int iSize>
int CHashTab<iSize>::Initialize()
{
	int i;

	m_iHashGetNumber = 0;
	m_iHashSearchNumber = 0;
	m_iHashNullNumber = 0;
	
	for( i = 0; i < iSize; i++ )
	{
		m_astHashItems[i].m_iFirstObjIdx = -1;
	}

	return 0;
}

template<int iSize>
int CHashTab<iSize>::Initialize(CObjSeg *pObjMng, short nHashNext)
{
	m_pObjMng = pObjMng;
	m_nHashNext = nHashNext;
	
	Initialize();
	return 0;
}

template<int iSize>
int CHashTab<iSize>::Resume(CObjSeg *pObjMng)
{
	m_pObjMng = pObjMng;
	return 0;
}

template<int iSize>
THashItem& CHashTab<iSize>::operator []( int iIdx )
{
	return m_astHashItems[iIdx];
}

template<int iSize>
CObj* CHashTab<iSize>::CreateObjectByKey(const void *pvKey, int iKeyLength, int& iErrNo, int *pNew)
{
	int iTempIdx = -1;
	int iHashIdx = -1;
	
	BYTE abyTempKey[1024];
	int  iTempKeyLen = 0;

	CIdx *pTempIdx = NULL;

	iErrNo = 0;		

	if( !pvKey || iKeyLength <= 0 || !m_pObjMng )
	{
		iErrNo = -1;
		return NULL;
	}

	iHashIdx = HashKeyToIdx(pvKey, iKeyLength);

	if( iHashIdx < 0 )
	{
		iErrNo = -2;
		return NULL;
	}

	iHashIdx %= iSize;

	//没有发生冲突，直接分配该节点
	if( m_astHashItems[iHashIdx].m_iFirstObjIdx >= 0 )
	{
		//查找有无相同节点存在
		iTempIdx = m_astHashItems[iHashIdx].m_iFirstObjIdx;
		while( iTempIdx >= 0 )
		{
			pTempIdx = m_pObjMng->GetIdx(iTempIdx);
			if( !pTempIdx )
			{
				iErrNo = -3;
				return NULL;
			}

			pTempIdx->GetAttachedObj()->GetHashKey((void *)abyTempKey, iTempKeyLen);
			if( iTempKeyLen == iKeyLength &&
				!memcmp((const void *)abyTempKey, pvKey, iKeyLength) )
			{
				break;
			}

			pTempIdx->GetDsInfo(m_nHashNext, &iTempIdx);
		}

		//如果该节点已经存在，则直接返回找到的节点
		if( iTempIdx >= 0 )
		{
			if(NULL != pNew)
			{
				*pNew = USEEXISTED;
			}
			return m_pObjMng->GetObj(iTempIdx);
		}
	}
	
	//否则创建节点，并挂入冲突列表头
	iTempIdx = m_pObjMng->CreateObject();
	if( iTempIdx < 0 )
	{
		iErrNo = -4;
		return NULL;
	}

	pTempIdx = m_pObjMng->GetIdx(iTempIdx);
	pTempIdx->SetDsInfo(m_nHashNext, m_astHashItems[iHashIdx].m_iFirstObjIdx);
	m_astHashItems[iHashIdx].m_iFirstObjIdx = iTempIdx;

	pTempIdx->GetAttachedObj()->SetHashKey(pvKey, iKeyLength);

	if(NULL != pNew)
	{
		*pNew = NEWCREATED;
	}
	return m_pObjMng->GetObj(iTempIdx);
}

template<int iSize>
CObj* CHashTab<iSize>::GetObjectByKey(const void *pvKey, int iKeyLength)
{
	int iTempIdx = -1;
	int iHashIdx = -1;
	
	BYTE abyTempKey[1024];
	int  iTempKeyLen = 0;

	CIdx *pTempIdx = NULL;

	if( !pvKey || iKeyLength <= 0 || !m_pObjMng )
	{
		return NULL;
	}
	
	m_iHashGetNumber++;
	
	iHashIdx = HashKeyToIdx(pvKey, iKeyLength);

	if( iHashIdx < 0 )
	{
		m_iHashNullNumber++;
		return NULL;
	}

	iHashIdx %= iSize;

	if( m_astHashItems[iHashIdx].m_iFirstObjIdx < 0 )
	{
		m_iHashNullNumber++;
		return NULL;
	}

	//查找有无相同节点存在
	iTempIdx = m_astHashItems[iHashIdx].m_iFirstObjIdx;
	while( iTempIdx >= 0 )
	{
		m_iHashSearchNumber++;
		
		pTempIdx = m_pObjMng->GetIdx(iTempIdx);
		if( !pTempIdx )
		{
			m_iHashNullNumber++;
			return NULL;
		}

		pTempIdx->GetAttachedObj()->GetHashKey((void *)abyTempKey, iTempKeyLen);
		if( iTempKeyLen == iKeyLength &&
			!memcmp((const void *)abyTempKey, pvKey, iKeyLength) )
		{
			break;
		}

		pTempIdx->GetDsInfo(m_nHashNext, &iTempIdx);
	}

	//如果该节点存在，则返回找到的节点
	if( iTempIdx >= 0 )
	{
		return m_pObjMng->GetObj(iTempIdx);
	}
	else
	{
		m_iHashNullNumber++;
		return NULL;
	}
}

template<int iSize>
int CHashTab<iSize>::DeleteObjectByKey(const void *pvKey, int iKeyLength)
{
	int iTempIdx = -1;
	int iTempPrevIdx = -1;
	int iTempNextIdx = -1;

	int iHashIdx = -1;
	
	BYTE abyTempKey[1024];
	int  iTempKeyLen = 0;

	CIdx *pTempIdx = NULL;
	CIdx *pTempPrevIdx = NULL;

	if( !pvKey || iKeyLength <= 0 || !m_pObjMng )
	{
		return -1;
	}

	iHashIdx = HashKeyToIdx(pvKey, iKeyLength);

	if( iHashIdx < 0 )
	{
		return -2;
	}

	iHashIdx %= iSize;

	if( m_astHashItems[iHashIdx].m_iFirstObjIdx < 0 )
	{
		return -3;
	}

	//查找有无相同节点存在
	iTempIdx = m_astHashItems[iHashIdx].m_iFirstObjIdx;
	while( iTempIdx >= 0 )
	{
		pTempIdx = m_pObjMng->GetIdx(iTempIdx);
		if( !pTempIdx )
		{
			return -4;
		}

		pTempIdx->GetAttachedObj()->GetHashKey((void *)abyTempKey, iTempKeyLen);
		if( iTempKeyLen == iKeyLength &&
			!memcmp((const void *)abyTempKey, pvKey, iKeyLength) )
		{
			break;
		}

		iTempPrevIdx = iTempIdx;
		pTempIdx->GetDsInfo(m_nHashNext, &iTempIdx);
	}

	if( iTempIdx < 0 )
	{
		return -5;
	}
	
	//如果该节点存在，则从哈希表中删除找到的节点
	pTempIdx->GetDsInfo(m_nHashNext, &iTempNextIdx);
	pTempPrevIdx = m_pObjMng->GetIdx(iTempPrevIdx);
	if( pTempPrevIdx )
	{
		pTempPrevIdx->SetDsInfo(m_nHashNext, iTempNextIdx);
	}
	pTempIdx->SetDsInfo(m_nHashNext, -1);

	if( iTempIdx == m_astHashItems[iHashIdx].m_iFirstObjIdx )
	{
		m_astHashItems[iHashIdx].m_iFirstObjIdx = iTempNextIdx;
	}

	return iTempIdx;
}

template<int iSize>
int CHashTab<iSize>::HashKeyToIdx(const void *pvKey, int iKeyLength)
{
	int iTempIdx = -1;
	unsigned int iHashIdx = 0;
	unsigned int iTempInt = 0;
	BYTE *pByte = NULL;
	int i;

	unsigned int *piTemp = (unsigned int *)pvKey;

	for( i = 0; i < (int)(iKeyLength/sizeof(unsigned int)); i++ )
	{
		iHashIdx += piTemp[i];
	}

	if( iKeyLength%sizeof(unsigned int) > 0 )
	{
		pByte = (BYTE *)pvKey;
		pByte += iKeyLength-(iKeyLength%sizeof(unsigned int));
		memcpy((void *)&iTempInt, (const void *)pByte, iKeyLength%sizeof(unsigned int));
	}

	iHashIdx += iTempInt;
	
	iTempIdx = (int)(iHashIdx & ((unsigned int)0x7fffffff));

	iTempIdx %= iSize;

	return iTempIdx;
}

#endif
