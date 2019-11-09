
#ifndef __MYLINK_H__
#define __MYLINK_H__

namespace svrlib
{
	struct CMyLinkNode
	{
		CMyLinkNode * pPrev;
		CMyLinkNode * pNext;
		CMyLinkNode()
		{
			pPrev = pNext = NULL;
		}
		inline void SetPrev(CMyLinkNode * pNode)
		{
			if(pNode)
			{
				pNode->pNext = this;
			}
			pPrev = pNode;
		}
		inline void SetNext(CMyLinkNode * pNode)
		{
			if(pNode)
			{
				pNode->pPrev = this;
			}
			pNext = pNode;
		}
	};

	class CMyLink
	{
	public:
		CMyLinkNode * m_pHead;
		CMyLinkNode * m_pTail;
		CMyLink()
		{
			m_pHead = m_pTail = NULL;
		}

		void Add2Head(CMyLinkNode * pNode)
		{
			if(!pNode)
			{
				return;
			}
			pNode->SetNext(m_pHead);
			m_pHead = pNode;
			if(!m_pTail)
			{
				m_pTail = m_pHead;
			}
		}
		void Add2Tail(CMyLinkNode * pNode)
		{
			if(!pNode)
			{
				return;
			}
			pNode->SetPrev(m_pTail);
			m_pTail = pNode;
			if(!m_pHead)
			{
				m_pHead = m_pTail;
			}
		}
		void Remove(CMyLinkNode * pNode)
		{
			if(!pNode)
			{
				return;
			}
			if(pNode == m_pHead)
			{
				m_pHead = pNode->pNext;
			}

			if(pNode == m_pTail)
			{
				m_pTail = pNode->pPrev;
			}

			if(pNode->pPrev)
			{
				pNode->pPrev->SetNext(pNode->pNext);
			}
			else if(pNode->pNext)
			{
				pNode->pNext->SetPrev(NULL);
			}
			
			pNode->pPrev = pNode->pNext = NULL;
		}
		void Move2Head(CMyLinkNode * pNode)
		{
			Remove(pNode);
			Add2Head(pNode);
		}
		void Move2Tail(CMyLinkNode * pNode)
		{
			Remove(pNode);
			Add2Tail(pNode);
		}
	};
}

#endif // __MYLINK_H__

