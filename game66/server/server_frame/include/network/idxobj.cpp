#ifndef _IDXOBJ_CPP_
#define _IDXOBJ_CPP_

#include "idxobj.h"
#include <stdlib.h>

//CSharedMem* CObjSeg::pCurrentShm = NULL;

//extern CSharedMem *g_pShmCore;

//IMPLEMENT_DYN(CObj)

byte* CObj::pCurrentObj = NULL;
int CObj::nCreateMode = 0;

void* CObj::operator new( size_t nSize ) throw()
{
    unused(nSize);
	if( !pCurrentObj )
	{
		return NULL;
	}

	return (void *)pCurrentObj;
}

void  CObj::operator delete( void *pMem )
{
    unused(pMem);
	return;
}

int CObj::GetObjectID()
{
	return m_iObjectID;
}

void* CObjSeg::operator new(size_t nSize) throw()
{
#if 0
	char *pTemp;

	if( !pCurrentShm )
	{
		return NULL;
	}

	pTemp = (char *)(pCurrentShm->CreateSegment(nSize));
#endif
	char *pTemp;

	pTemp =(char *) malloc(nSize);
	return (void *)pTemp;
}

void CObjSeg::operator delete( void *pMem )
{
	free(pMem);
	pMem = NULL;
	return;
}

size_t CObjSeg::CountSegSize(size_t nObjSize, int iItemCount)
{
	size_t iTempSize = 0;

	iTempSize += sizeof(CObjSeg);
	iTempSize += iItemCount*sizeof(CIdx);
	iTempSize += iItemCount*nObjSize;

	return iTempSize;
}

CObjSeg::CObjSeg(size_t nObjSize, int iItemCount, CObj* (*pfCreateObj)(void *))
{
#if 0
	if( !pCurrentShm )
	{
		return;
	}

	if( pCurrentShm->GetInitMode() == Init )
	{
		m_nObjSize = nObjSize;
		m_iItemCount = iItemCount;
	}
#endif
	m_nObjSize = nObjSize;
	m_iItemCount = iItemCount;

#if 0
	m_pIdxs = (CIdx *)(pCurrentShm->CreateSegment(m_iItemCount*sizeof(CIdx)));
	//CSharedMem::m_pbNextSeg += m_iItemCount*sizeof(CIdx);

	m_pObjs = (CObj *)(pCurrentShm->CreateSegment(m_iItemCount*m_nObjSize));
	//CSharedMem::m_pbNextSeg += m_iItemCount*m_nObjSize;
	if( !m_pIdxs || !m_pObjs)
	{
		return;
	}
#endif
	m_pIdxs= (CIdx*)malloc(m_iItemCount*sizeof(CIdx));

	m_pObjs = (CObj*)malloc(m_iItemCount*m_nObjSize);

	if( !m_pIdxs || !m_pObjs)
	{
		return;
	}

	m_pfNew = pfCreateObj;

	Init();
	CObj::nCreateMode = 0;
#if 0

	if( pCurrentShm->GetInitMode() == Init )
	{
		Init();
		CObj::nCreateMode = 0;
	}
	else
	{
		RecoverIdx();
		CObj::nCreateMode = 1;
	}
#endif
	FormatObj();
}

CObjSeg::~CObjSeg()
{
}

int CObjSeg::Init()
{
	m_iIsFormated = 0;
	m_iFreeHead = -1;
	m_iUsedTail = -1;
	m_iUsedHead = -1;
	m_iUsedCount = 0;

	if( FormatIdx() )
	{
		return -1;
	}

	m_iIsFormated = 1;

	return 0;
}

int CObjSeg::FormatObj()
{
	CObj *pTempObj;

	if( !m_pfNew )
	{
		return -1;
	}

	//每个idx上挂上一个obj对象指针
	for( int i = 0 ; i < m_iItemCount; i++ )
	{
		pTempObj = (*m_pfNew)((void *)((char*)m_pObjs + m_nObjSize*i));
		pTempObj->m_iObjectID = i;
		m_pIdxs[i].SetAttachedObj(pTempObj);
	}

	return 0;
}
int CObjSeg::FormatIdx()
{
	m_iFreeHead = 0;
	m_iUsedHead = -1;
	m_iUsedTail = -1;
	int i;

	m_pIdxs[m_iFreeHead].Init();
	m_pIdxs[m_iFreeHead].SetPrevIdx(-1);

	//将对象链表串起来
	for( i = 1 ; i < m_iItemCount; i++ )
	{
		m_pIdxs[i].Init();
		m_pIdxs[i-1].SetNextIdx(i);
		m_pIdxs[i].SetPrevIdx(i-1);
	}

	m_pIdxs[m_iItemCount-1].SetNextIdx(-1);

	//m_pIdxs[m_iFreeHead].SetPrevIdx(-1);

	return 0;
}

int CObjSeg::RecoverIdx()
{
	return 0;
}

int CObjSeg::GetItemCount()
{
	return m_iItemCount;
}

int CObjSeg::GetUsedCount()
{
	return m_iUsedCount;
}
int	CObjSeg::GetFreeCount()
{
	return  (m_iItemCount-m_iUsedCount);
}

int CObjSeg::GetUsedHead()
{
	return m_iUsedHead;
}
int CObjSeg::GetFreeHead()
{
	return m_iFreeHead;
}

int CObjSeg::GetUsedTail()
{
	return m_iUsedTail;
}

CIdx* CObjSeg::GetIdx(int iIdx)
{
	if( iIdx < 0 || iIdx >= m_iItemCount )
	{
		return NULL;
	}

	if( !m_pIdxs[iIdx].IsUsed() )
	{
		return NULL;
	}

	return &m_pIdxs[iIdx];
}

CObj* CObjSeg::GetObj(int iIdx)
{
	if( iIdx < 0 || iIdx >= m_iItemCount )
	{
		return NULL;
	}

	if( !m_pIdxs[iIdx].IsUsed() )
	{
		return NULL;
	}

	return m_pIdxs[iIdx].GetAttachedObj();
}

int CObjSeg::CreateObject()
{
	//从链表中找到一个空闲的对象，挂到已用对象列表中
	int iTempIdx;
	if( m_iUsedCount >= m_iItemCount )
	{
		//CWriteRunInfo::WriteErrLog("m_iUsedCount %d >= m_iItemCount %d", m_iUsedCount, m_iItemCount  );
		return -1;
	}

	iTempIdx = m_iFreeHead;

	m_iFreeHead = m_pIdxs[iTempIdx].GetNextIdx();

	if( m_iFreeHead >= 0 )
	{
		m_pIdxs[m_iFreeHead].SetPrevIdx(-1);
	}

	m_pIdxs[iTempIdx].SetUsed();
	m_pIdxs[iTempIdx].SetNextIdx(-1);
	m_pIdxs[iTempIdx].SetPrevIdx(m_iUsedTail);
	m_pIdxs[iTempIdx].SetUsedNextIdx(-1);
	m_pIdxs[iTempIdx].SetUsedPrevIdx(-1);
	if( m_iUsedTail >= 0 )
	{
		m_pIdxs[m_iUsedTail].SetNextIdx(iTempIdx);
	}
	if(-1 == m_iUsedHead)
	{
		m_iUsedHead = iTempIdx;
	}
	m_iUsedTail = iTempIdx;
	CObj * pObj = NULL;
	pObj = m_pIdxs[iTempIdx].GetAttachedObj();
	if(pObj)
	{
		pObj->Init();
	}

	m_iUsedCount++;

	return iTempIdx;
}

int CObjSeg::DestroyObject(int iIdx)
{
	if( iIdx >= m_iItemCount || iIdx < 0 || m_iItemCount <= 0 )
	{
		return -1;
	}

	if( !m_pIdxs[iIdx].IsUsed() )
	{
		return -2;
	}

	if(iIdx == m_iUsedHead)
	{
		m_iUsedHead = m_pIdxs[iIdx].GetNextIdx();
	}
	//从使用队列中删除
	if( iIdx == m_iUsedTail )
	{
		m_iUsedTail = m_pIdxs[iIdx].GetPrevIdx();
		if( m_iUsedTail >= 0 )
		{
			m_pIdxs[m_iUsedTail].SetNextIdx(-1);
		}
	}
	if( m_pIdxs[iIdx].GetNextIdx() >= 0 )
	{
		m_pIdxs[m_pIdxs[iIdx].GetNextIdx()].SetPrevIdx(m_pIdxs[iIdx].GetPrevIdx());
	}
	if( m_pIdxs[iIdx].GetPrevIdx() >= 0 )
	{
		m_pIdxs[m_pIdxs[iIdx].GetPrevIdx()].SetNextIdx(m_pIdxs[iIdx].GetNextIdx());
	}

	//从使用队列中删除
	//不是第一个节点
	if(-1 != m_pIdxs[iIdx].GetUsedPrevIdx())
	{
		//是最后一个节点
		if(-1 == m_pIdxs[iIdx].GetUsedNextIdx())
		{
			m_pIdxs[m_pIdxs[iIdx].GetUsedPrevIdx()].SetUsedNextIdx(-1);
		}
		//不是最后一个节点
		else
		{
			m_pIdxs[m_pIdxs[iIdx].GetUsedPrevIdx()].SetUsedNextIdx(m_pIdxs[iIdx].GetUsedNextIdx());
			m_pIdxs[m_pIdxs[iIdx].GetUsedNextIdx()].SetUsedPrevIdx(m_pIdxs[iIdx].GetUsedPrevIdx());
		}
	}
	//是第一个节点
	else
	{
		//且是最后一个节点
		if(-1 == m_pIdxs[iIdx].GetUsedNextIdx())
		{
		}
		//不是最后一个节点
		else
		{
			m_pIdxs[m_pIdxs[iIdx].GetUsedNextIdx()].SetUsedPrevIdx(-1);
		}
	}

	//挂入空闲队列待分配
	m_pIdxs[iIdx].GetAttachedObj()->FInit();
	m_pIdxs[iIdx].SetUsedNextIdx(-1);
	m_pIdxs[iIdx].SetUsedPrevIdx(-1);
	m_pIdxs[iIdx].SetFree();
	m_pIdxs[iIdx].SetPrevIdx(-1);
	m_pIdxs[iIdx].SetNextIdx(m_iFreeHead);
	if( m_iFreeHead >= 0 )
	{
		m_pIdxs[m_iFreeHead].SetPrevIdx(iIdx);
	}
	m_iFreeHead = iIdx;

	m_iUsedCount--;

	return iIdx;
}

//这个函数用于将新的节点挂接到链表头部使用
int CObjSeg::ModifyRelation(int iNewIdx, int iCurrent)
{
	if( iNewIdx >= m_iItemCount || iNewIdx < 0 || m_iItemCount <= 0 )
	{
		return -1;
	}

	if( 0 == m_pIdxs[iNewIdx].IsUsed())
	{
		return -2;
	}
	if(iCurrent >=0 && !m_pIdxs[iCurrent].IsUsed())
	{
		return -3;
	}

	if(-1 == iCurrent)
	{
		m_pIdxs[iNewIdx].SetUsedPrevIdx(-1);
		m_pIdxs[iNewIdx].SetUsedNextIdx(-1);
	}
	else
	{
		m_pIdxs[iNewIdx].SetUsedPrevIdx(-1);
		m_pIdxs[iNewIdx].SetUsedNextIdx(iCurrent);
		m_pIdxs[iCurrent].SetUsedPrevIdx(iNewIdx);
	}

	return 0;
}

//用于将节点放到某个节点头部
int CObjSeg::AddToPre(int iNewIdx, int iCurrent)
{
	if( iNewIdx >= m_iItemCount || iNewIdx < 0 || m_iItemCount <= 0 ||  iNewIdx == iCurrent)
	{
		return -1;
	}

	if( 0 == m_pIdxs[iNewIdx].IsUsed())
	{
		return -2;
	}
	if(iCurrent >=0 && !m_pIdxs[iCurrent].IsUsed())
	{
		return -3;
	}

	//先将节点从原有链表中取下
	//判断新增加节点前后是否存在节点
	int iNewPreIdx = m_pIdxs[iNewIdx].GetUsedPrevIdx();
	int iNewNextIdx = m_pIdxs[iNewIdx].GetUsedNextIdx();
	if(-1 != iNewPreIdx)
	{
		m_pIdxs[iNewPreIdx].SetUsedNextIdx(iNewNextIdx);
	}
	if(-1 != iNewNextIdx)
	{
		m_pIdxs[iNewNextIdx].SetUsedPrevIdx(iNewPreIdx);
	}

	if(-1 == iCurrent)
	{
		m_pIdxs[iNewIdx].SetUsedPrevIdx(-1);
		m_pIdxs[iNewIdx].SetUsedNextIdx(-1);
	}
	else
	{
		//检查当前节点前后是否存在节点
		int iCurPreIdx = m_pIdxs[iCurrent].GetUsedPrevIdx();
		m_pIdxs[iNewIdx].SetUsedPrevIdx(iCurPreIdx);
		if(-1 != iCurPreIdx)
		{
			m_pIdxs[iCurPreIdx].SetUsedNextIdx(iNewIdx);
		}

		m_pIdxs[iNewIdx].SetUsedNextIdx(iCurrent);
		m_pIdxs[iCurrent].SetUsedPrevIdx(iNewIdx);
	}

	return 0;
}

//用于将节点放到尾部
int CObjSeg::AddToNext(int iNewIdx, int iCurrent)
{
	if( iNewIdx >= m_iItemCount || iNewIdx < 0 || m_iItemCount <= 0 ||  iNewIdx == iCurrent)
	{
		return -1;
	}

	if( 0 == m_pIdxs[iNewIdx].IsUsed())
	{
		return -2;
	}
	if(iCurrent >=0 && !m_pIdxs[iCurrent].IsUsed())
	{
		return -3;
	}

	//先将节点从原有链表中取下
	//判断新增加节点前后是否存在节点
	int iNewPreIdx = m_pIdxs[iNewIdx].GetUsedPrevIdx();
	int iNewNextIdx = m_pIdxs[iNewIdx].GetUsedNextIdx();

	if(-1 != iNewPreIdx)
	{
		m_pIdxs[iNewPreIdx].SetUsedNextIdx(iNewNextIdx);
	}
	if(-1 != iNewNextIdx)
	{
		m_pIdxs[iNewNextIdx].SetUsedPrevIdx(iNewPreIdx);
	}

	if(-1 == iCurrent)
	{
		m_pIdxs[iNewIdx].SetUsedPrevIdx(-1);
		m_pIdxs[iNewIdx].SetUsedNextIdx(-1);
	}
	else
	{
		//检查当前节点前后是否存在节点
		int iCurNextIdx = m_pIdxs[iCurrent].GetUsedNextIdx();
		m_pIdxs[iNewIdx].SetUsedNextIdx(iCurNextIdx);
		if(-1 != iCurNextIdx)
		{
			m_pIdxs[iCurNextIdx].SetUsedPrevIdx(iNewIdx);
		}

		m_pIdxs[iNewIdx].SetUsedPrevIdx(iCurrent);
		m_pIdxs[iCurrent].SetUsedNextIdx(iNewIdx);
	}

	return 0;
}

CIdx::CIdx()
{
	Init();
}

CIdx::~CIdx()
{
}

void CIdx::Init()
{
	m_iNextIdx = -1;
	m_iPrevIdx = -1;

	m_iUsedNext = -1;
	m_iUsedPrev = -1;

	m_iUseFlag = 0;

	m_pAttachedObj = NULL;
	/*
	for( int i = 0; i < MAXDSINFOITEM; i++ )
	{
	m_piDsInfo[i] = -1;
	}*/
}

int CIdx::GetNextIdx()
{
	return m_iNextIdx;
}

void CIdx::SetNextIdx(int iIdx)
{
	m_iNextIdx = iIdx;
	return;
}

int CIdx::GetPrevIdx()
{
	return m_iPrevIdx;
}

void CIdx::SetPrevIdx(int iIdx)
{
	m_iPrevIdx = iIdx;
	return;
}

int	CIdx::GetUsedPrevIdx()
{
	return m_iUsedPrev;
}
void CIdx::SetUsedPrevIdx(int iIdx)
{
	m_iUsedPrev = iIdx;
}

int CIdx::GetUsedNextIdx()
{
	return m_iUsedNext;
}

void CIdx::SetUsedNextIdx(int iIdx)
{
	m_iUsedNext = iIdx;
}

/*
int CIdx::GetHashNextIdx()
{
return m_iHashNextIdx;
}
void CIdx::SetHashNextIdx(int iIdx)
{
m_iHashNextIdx = iIdx;
return;
}

int CIdx::GetHashPrevIdx()
{
return m_iHashPrevIdx;
}
void CIdx::SetHashPrevIdx(int iIdx)
{
m_iHashPrevIdx = iIdx;
return;
}
*/

void CIdx::SetUsed()
{
	m_iUseFlag = 1;
	return;
}
void CIdx::SetFree()
{
	m_iUseFlag = 0;
	return;
}
int CIdx::IsUsed()
{
	return m_iUseFlag;
}

CObj* CIdx::GetAttachedObj()
{
	return m_pAttachedObj;
}

int CIdx::SetAttachedObj(CObj *pObj)
{
	if( !pObj )
	{
		return -1;
	}

	m_pAttachedObj = pObj;

	return 0;
}

int CIdx::GetDsInfo(short nDsIdx, int *piDsVal)
{
	if( nDsIdx < 0 || nDsIdx >= MAXDSINFOITEM || piDsVal == NULL )
	{
		return -1;
	}
	*piDsVal = m_piDsInfo[nDsIdx];
	return 0;
}

int CIdx::SetDsInfo(short nDsIdx, int iDsVal)
{
	if( nDsIdx < 0 || nDsIdx >= MAXDSINFOITEM )
	{
		return -1;
	}

	m_piDsInfo[nDsIdx] = iDsVal;
	return 0;
}

#endif
