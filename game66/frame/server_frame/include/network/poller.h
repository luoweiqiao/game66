#ifndef __POLLER_H__
#define __POLLER_H__

#include <arpa/inet.h>
#include <myepoll.h>
#include <list.h>

#define EPOLL_DATA_SLOT(x)	((x)->data.u64 & 0xFFFFFFFF)
#define EPOLL_DATA_SEQ(x)	((x)->data.u64 >> 32)

enum TPollerState
{
    POLLER_FAIL     = -1,
    POLLER_SUCC,
    POLLER_COMPLETE
};

class CPollerUnit;
class CPollerObject;

struct CEpollSlot 
{
	uint32_t seq;
	CPollerObject *poller;
	struct CEpollSlot *freeList;
};

class CPollerObject 
{
public:
	CPollerObject (CPollerUnit *thread=NULL, int fd=0);
	virtual ~CPollerObject ();

	virtual int DestroyObj (){return 0;}
	
	virtual int Init()=0;
	virtual int InputNotify (void);
	virtual int OutputNotify (void);
	virtual int HangupNotify (void);
	
	void EnableInput(void) 
	{
		newEvents |= EPOLLIN|EPOLLERR | EPOLLHUP;
		 
	}
	void EnableOutput(void) 
	{
		newEvents |= EPOLLOUT|EPOLLERR | EPOLLHUP;
	}
	void DisableInput(void) 
	{
		newEvents &= ~EPOLLIN;
	}
	void DisableOutput(void) 
	{
        //log_debug ("netfd[%d] DisableOutput!!!", netfd);
		newEvents &= ~EPOLLOUT;
	}

	void EnableInput(bool i) 
	{
		if(i)
			newEvents |= EPOLLIN|EPOLLERR | EPOLLHUP;
		else
			newEvents &= ~EPOLLIN;
	}
	void EnableOutput(bool o) 
	{
		if(o)
			newEvents |= EPOLLOUT|EPOLLERR | EPOLLHUP;
		else
			newEvents &= ~EPOLLOUT;
	}

	int AttachPoller (CPollerUnit *thread=NULL);
	int DetachPoller (void);
	int ApplyEvents ();
	uint64_t GetPollerEventId();

	friend class CPollerUnit;
	int InitObj();

protected:
	int netfd;
	CPollerUnit *ownerUnit;
	int newEvents;
	int oldEvents;
	struct CEpollSlot *epslot;
};

class CPollerUnit 
{
public:
	friend class CPollerObject;
	CPollerUnit(int mp);
	~CPollerUnit();

	int SetMaxPollers(int mp);
	int GetMaxPollers(void) const { return maxPollers; }
	int InitializePollerUnit(void);
	void WaitPollerEvents(int);
	void ProcessPollerEvents(void);
	int GetFD(void) { return epfd; }
	int GetSlotId (CEpollSlot *p) {return ((char*)p - (char*)pollerTable) / sizeof (CEpollSlot);}

private:
	int VerifyEvents(struct epoll_event *);
	int Epctl (int op, int fd, struct epoll_event *events);


	void FreeEpollSlot (CEpollSlot *p);
	struct CEpollSlot *AllocEpollSlot ();

	struct epoll_event *ep_events;
	int epfd;
	int maxPollers;
	int usedPollers;
	struct CEpollSlot *freeSlotList;
	struct CEpollSlot *pollerTable;

	int nrEvents;
};

#endif
