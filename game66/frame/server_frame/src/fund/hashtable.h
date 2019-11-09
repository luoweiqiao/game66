#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H
#include <vector>

namespace fund
{

inline unsigned int GetNextPrime(unsigned int iValue)
{
    const unsigned int primeList[] = 
    {
        53,         97,         193,       389,       769, 
		1543,       3079,       6151,      12289,     24593, 
		49157,      98317,      196613,    393241,    786433, 
		1572869,    3145739,    6291469,   12582917,  25165843, 
		50331653,   100663319,  201326611, 402653189, 805306457, 
		1610612741, 3221225473ul, 4294967291ul
    };
	
    unsigned int iPrimeNum = sizeof(primeList) / sizeof(unsigned int);
	
    for (unsigned int i=0; i<iPrimeNum; ++i)
    {
        if (primeList[i] >= iValue)
        {
            return primeList[i];
        }
    }
	
    return primeList[iPrimeNum - 1];
}

template <typename NodeData>
struct PoolNode
{
	typedef PoolNode<NodeData> this_type;

	PoolNode()
		: pre(0), next(0) 
	{}

	~PoolNode()
	{}

	inline NodeData* operator -> () 
	{
		return data;
	}

	inline const NodeData* operator -> () const 
	{
		return data;
	}

	inline NodeData& operator * () 
	{
		return *data;
	}

	inline const NodeData & operator * () const 
	{
		return *data;
	}

	inline bool operator == (const this_type& pNode) const 
	{
		return data == pNode->data;
	}
	
	inline bool operator == (const NodeData& nodeData) const 
	{
		return data == nodeData;
	}

	inline bool operator != (const NodeData& nodeData) const 
	{
		return !operator == (nodeData);
	}

	inline bool operator < (const NodeData& nodeData) const 
	{
		return data < nodeData;
	}

	inline bool operator <= (const NodeData& nodeData) const 
	{
		return operator < (nodeData) || operator == (nodeData);
	}

	inline bool operator > (const NodeData& nodeData) const 
	{
		return !operator <= (nodeData);
	}

	inline bool operator >= (const NodeData& nodeData) const 
	{
		return !operator < (nodeData);
	}

	NodeData data;
	this_type * pre;
	this_type * next;
}; //end struct PoolNode

template <typename Node>
class PoolNodeAllocator
{
public:
	explicit PoolNodeAllocator(unsigned int size)
		: mSize(size), mNodePool(new Node[size]), mUsedCnt(0), mUsedHead(0), mFreeHead(0) 
	{
		if (size > 0)
		{
			mFreeHead =  mNodePool;
			Node * cur = mFreeHead;
			for (unsigned int i = 0; i < size - 1; ++i, ++cur)
			{
				cur->next = cur + 1;
				cur->next->pre = cur;
			}
		}
	}

	~PoolNodeAllocator() {
		delete []mNodePool;
	}

	Node * Alloc() 
	{
		Node * node = mFreeHead;
		if (mFreeHead)
		{
			mFreeHead = mFreeHead->next;
			if (mUsedHead)
			{
				mUsedHead->pre = node;
			}
			node->next = mUsedHead;
			node->pre = 0;
			mUsedHead = node;
			++mUsedCnt;
		}
		return node;
	}

	bool Free(Node * node) 
	{
		if (node)
		{
			if (mUsedHead == node) mUsedHead = node->next;
			if (node->next) node->next->pre = node->pre;
			if (node->pre) node->pre->next = node->next;
			if (mFreeHead)
			{
				mFreeHead->pre = node;
			}
			node->next = mFreeHead;
			node->pre = 0;
			mFreeHead = node;
			--mUsedCnt;
			return true;
		}
		return false;
	}

	inline unsigned int Capacity() const 
	{
		return mSize;
	}

	inline unsigned int Size() const 
	{
		return mUsedCnt;
	}

	inline const Node * FirstUsedNode() const 
	{
		return mUsedHead;
	}

	inline Node * FirstUsedNode() 
	{
		return mUsedHead;
	}

	inline const Node * FirstUnusedNode() const 
	{
		return mFreeHead;
	}

	inline Node * FirstUnusedNode() 
	{
		return mFreeHead;
	}

private:
	const unsigned int mSize;
	unsigned int mUsedCnt;
	Node * mNodePool;
	Node * mUsedHead;
	Node * mFreeHead;
};//end class NodeAlloc



struct UintHashProxy
{
	inline UintHashProxy(unsigned int id = 0)
		: id_(id) {
	}

	inline unsigned int Hash() const {
		return id_;
	}

	inline bool operator == (const UintHashProxy& rhs) const {
		return id_ == rhs.id_;
	}

	inline bool operator == (unsigned int v) const {
		return id_ == v;
	}

	inline operator unsigned int () const {
		return id_;
	}
private:
	unsigned int id_;
};

template <typename VALUE, typename KEY>
struct ListNode
{
	KEY key;
	VALUE value;
	typedef ListNode<VALUE, KEY> LISTNODE;
	LISTNODE * pre;
	LISTNODE * next;
	PoolNode<LISTNODE>* poolnode;

	ListNode()
		: pre(0), next(0), poolnode(0) {
	}

	~ListNode() {
	}
	static KEY& GetKeyByVuale(const VALUE* pValue)
	{
		LISTNODE* pNode = (LISTNODE*)((char*)pValue-(unsigned long)(&((LISTNODE *)0)->value));
		return pNode->key;
	}
};

template <typename VALUE, typename KEY, typename Allocator = PoolNodeAllocator<PoolNode<ListNode<VALUE, KEY> > > >
class HashTblBucket
{
public:
	typedef ListNode<VALUE, KEY> LISTNODE;
	typedef PoolNode<LISTNODE > POOLNODE;

	HashTblBucket(Allocator& allocator)
		: mHeader(0), mAllocator(&allocator), mSize(0) 
	{}

	VALUE * Add(const KEY& key, const VALUE& val, bool& add) 
	{ //增加或更新
		LISTNODE * p = GetListNode(key);
		if (p) {//如果已经存在，就更新
			add = false;
			p->value = val;
			return &p->value;
		}
		//如果不存在，新增一个
		POOLNODE * node = mAllocator->Alloc();
		if (node) {
			node->data.value = val;
			node->data.key = key;
			node->data.poolnode = node;
			node->data.pre = 0;
			if (mHeader) {
				node->data.next = mHeader;
				mHeader->pre = &node->data;
			}
			else {
				node->data.next = 0;
			}
			mHeader = &node->data;
			++mSize;
			add = true;
			return &node->data.value;
		}
		return 0;
	}

	bool Del(const KEY& key) 
	{ //删除一个节点
		LISTNODE * p = GetListNode(key);
		if (p) {
			if (mHeader == p) {//如果是头结点
				mHeader = p->next;
			}
			if (p->pre) {
				p->pre->next = p->next;
			}
			if (p->next) {
				p->next->pre = p->pre;
			}
			p->pre = 0;
			p->next = 0;
			mAllocator->Free(p->poolnode);
			--mSize;
			return true;
		}
		return false;
	}
	
	template <typename PRED>
	bool DelIf(const KEY& key, PRED pred) 
	{ //删除一个节点
		LISTNODE * p = GetListNode(key);
		if (p && pred(p->value)) {
			if (mHeader == p) {//如果是头结点
				mHeader = p->next;
			}
			if (p->pre) {
				p->pre->next = p->next;
			}
			if (p->next) {
				p->next->pre = p->pre;
			}
			p->pre = 0;
			p->next = 0;
			mAllocator->Free(p->poolnode);
			--mSize;
			return true;
		}
		return false;
	}

	inline VALUE * Get(const KEY& key) 
	{
		LISTNODE * p = GetListNode(key);
		if (p) {
			return &p->value;
		}
		return 0;
	}

	inline LISTNODE * GetFirstNode() {
		return mHeader;
	}

	inline unsigned int Size() const {
		return mSize;
	}

private:
	inline static bool DefPred(const VALUE &) {
		return true;
	}
	
	inline LISTNODE * GetListNode(const KEY& key) 
	{
		for(LISTNODE *pNode = mHeader; pNode != NULL; pNode = pNode->next)
		{
			if (pNode->key == key)  //如果已经存在
				return pNode;
		}
		return NULL;
	}

private:
	LISTNODE * mHeader; //头指针
	Allocator * mAllocator;
	unsigned int mSize;
};

template <typename DATA, typename KEY = UCharKEY, typename ALLOCATOR = PoolNodeAllocator<PoolNode<ListNode<DATA, KEY> > > >
class HashTable //二维列表
{
public:
#ifdef WIN32
typedef unsigned __int64 UINT64 ;
#else
typedef unsigned long long UINT64 ;
#endif
	typedef HashTblBucket<DATA, KEY, ALLOCATOR> Bucket;
	typedef ListNode<DATA, KEY> LISTNODE;
	typedef PoolNode<LISTNODE> POOLNODE;

	HashTable(unsigned int capacity, unsigned int bucketCnt) 
		: mAllocator(new ALLOCATOR(capacity)), mbSelAlloc(true),
		mBucketCnt(GetNextPrime(bucketCnt)),
		mBucketVec(mBucketCnt, Bucket(*mAllocator)),
		mSize(0) {
	}
	
	HashTable(unsigned int bucketCnt, ALLOCATOR & allocator) 
		: mAllocator(&allocator), mbSelAlloc(false),
		mBucketCnt(GetNextPrime(bucketCnt)),
		mBucketVec(mBucketCnt, Bucket(*mAllocator)),
		mSize(0) {
	}
	
	~HashTable() {
		if (mbSelAlloc) {
			delete mAllocator;
		}
	}

	inline DATA * Add(const KEY& key, const DATA& data) {
		Bucket * bucket = &mBucketVec[key.Hash() % mBucketCnt];//GetBucket(key);
		bool add = false;
		DATA * p = bucket->Add(key, data, add);
		if (p && add) ++mSize;
		return p;
	}

	inline bool Del(const KEY& key) {
		Bucket * bucket = &mBucketVec[key.Hash() % mBucketCnt];//GetBucket(key);
		bool ret = bucket->Del(key);
		if (ret) --mSize;
		return ret;
	}

	inline KEY& GetKeyByValue(const DATA* pData) {
		return LISTNODE::GetKeyByVuale(pData);
	}


	template <typename PRED>
	inline bool DelIf(const KEY& key, PRED pred) {
		Bucket * bucket = &mBucketVec[key.Hash() % mBucketCnt];//GetBucket(key);
		bool ret = bucket->DelIf(key, pred);
		if (ret) --mSize;
		return ret;
	}

	inline DATA * Get(const KEY& key) {
		Bucket * bucket = &mBucketVec[key.Hash() % mBucketCnt];//GetBucket(key);
		return bucket->Get(key);
	}

	inline unsigned int Size() const {
		return mSize;
	}

	inline LISTNODE * GetFirstNode() {
		for (unsigned int i = 0; i < mBucketCnt; ++i) {
			if (mBucketVec[i].Capacity() > 0) {
				return mBucketVec[i].GetFirstNode();
			}
		}
		return 0;
	}

	inline LISTNODE * GetBucketFirstNode(unsigned int bucket) {
		if (bucket < mBucketCnt) return mBucketVec[bucket].GetFirstNode();
		else return 0;
	}

	inline unsigned int TotalBucket() const {
		return mBucketCnt;
	}

	inline POOLNODE * GetFirstPoolNode() {
		return mAllocator->FirstUsedNode();
	}

private:
	ALLOCATOR * mAllocator;
	bool mbSelAlloc;
	const unsigned int mBucketCnt; //桶数
	std::vector<Bucket> mBucketVec; //桶数组
	unsigned int mSize;
}; //end class FastList

} ;

#endif //_HASH_TABLE_H


