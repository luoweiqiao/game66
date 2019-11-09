#ifndef _NODE_ALLOC_H
#define _NODE_ALLOC_H
#include <vector>

namespace fund
{
template <typename T>
struct PoolNode
{
	typedef PoolNode<T> this_type;
	typedef T type;

	PoolNode()
		: pre_(0), next_(0) {
	}

	~PoolNode()
	{
		//delete v_;
	}

	inline T * operator -> () {
		return v_;
	}

	inline const T * operator -> () const {
		return v_;
	}

	inline T & operator * () {
		return *v_;
	}

	inline const T & operator * () const {
		return *v_;
	}

	inline bool operator == (const this_type& v) const {
		return v_ == v->v_;
	}
	
	inline bool operator == (const T& v) const {
		return v_ == v;
	}

	inline bool operator != (const T& v) const {
		return !operator == (v);
	}

	inline bool operator < (const T& v) const {
		return v_ < v;
	}

	inline bool operator <= (const T& v) const {
		return operator < (v) || operator == (v);
	}

	inline bool operator > (const T& v) const {
		return !operator <= (v);
	}

	inline bool operator >= (const T& v) const {
		return !operator < (v);
	}

	T v_;
	this_type * pre_;
	this_type * next_;
}; //end struct PoolNode

template <typename Node>
class PoolNodeAllocator
{
public:
	explicit PoolNodeAllocator(unsigned int size)
		: Size_(size), PoolData_(new Node[Size_]), UsedCount_(0), Inused_(0), Unused_(0) {
		if (size > 0)
		{
			Unused_ =  PoolData_;
			Node * cur = Unused_;
			for (unsigned int i = 0; i < size - 1; ++i, ++cur)
			{
				cur->next_ = cur + 1;
				cur->next_->pre_ = cur->next_;
			}
		}
	}

	~PoolNodeAllocator() {
		delete []PoolData_;
	}

	Node * Alloc() {
		Node * node = Unused_;
		if (Unused_)
		{
			Unused_ = Unused_->next_;
			if (Inused_)
			{
				Inused_->pre_ = node;
			}
			node->next_ = Inused_;
			node->pre_ = 0;
			Inused_ = node;
			++UsedCount_;
		}
		return node;
	}

	bool Free(Node * node) {
		if (node)
		{
			if (Inused_ == node) Inused_ = node->next_;
			if (node->next_) node->next_->pre_ = node->pre_;
			if (node->pre_) node->pre_->next_ = node->next_;
			node->next_ = Unused_;
			node->pre_ = 0;
			Unused_ = node;
			--UsedCount_;
			return true;
		}
		return false;
	}

	inline unsigned int Capacity() const {
		return Size_;
	}

	inline unsigned int Size() const {
		return UsedCount_;
	}

	inline const Node * FirstUsedNode() const {
		return Inused_;
	}

	inline Node * FirstUsedNode() {
		return Inused_;
	}

	inline const Node * FirstUnusedNode() const {
		return Unused_;
	}

	inline Node * FirstUnusedNode() {
		return Unused_;
	}

private:
	const unsigned int Size_;
	unsigned int UsedCount_;
	Node * PoolData_;
	Node * Inused_;
	Node * Unused_;
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

template <typename T, typename KEY>
struct ListNode
{
	T v_;
	ListNode<T, KEY> * pre_;
	ListNode<T, KEY> * next_;
	KEY key_;
	PoolNode<ListNode<T, KEY> > * node_;

	ListNode()
		: pre_(0), next_(0), node_(0) {
	}

	~ListNode() {
	}
};



template <typename T, typename KEY, typename Allocator = PoolNodeAllocator<PoolNode<ListNode<T, KEY> > > >
class FastListLine
{
public:
	typedef ListNode<T, KEY> LISTNODE;
	typedef PoolNode<LISTNODE > POOLNODE;

	explicit FastListLine(Allocator& allocator)
		: Header_(0), Allocator_(&allocator), Size_(0) {
	}

	T * Add(const KEY& key, const T& v, bool& add) { //增加或更新
		LISTNODE * p = GetListNode(key);
		if (p) {//如果已经存在，就更新
			add = false;
			p->v_ = v;
			return &p->v_;
		}
		//如果不存在，新增一个
		POOLNODE * node = Allocator_->Alloc();
		if (node) {
			node->v_.v_ = v;
			node->v_.key_ = key;
			node->v_.node_ = node;
			node->v_.pre_ = 0;
			if (Header_) {
				node->v_.next_ = Header_;
				Header_->pre_ = &node->v_;
			}
			else {
				node->v_.next_ = 0;
			}
			Header_ = &node->v_;
			++Size_;
			add = true;
			return &node->v_.v_;
		}
		return 0;
	}

	bool Del(const KEY& key) { //删除一个节点
#ifndef _MSC_VER
		return DelIf(key, DefPred);
#else
		LISTNODE * p = GetListNode(key);
		if (p) {
			if (Header_ == p) {//如果是头结点
				Header_ = p->next_;
			}
			if (p->pre_) {
				p->pre_->next_ = p->next_;
			}
			if (p->next_) {
				p->next_->pre_ = p->pre_;
			}
			p->pre_ = 0;
			p->next_ = 0;
			Allocator_->Free(p->node_);
			--Size_;
			return true;
		}
		return false;
#endif
	}
	
	template <typename PRED>
	bool DelIf(const KEY& key, PRED pred) { //删除一个节点
		LISTNODE * p = GetListNode(key);
		if (p && pred(p->v_)) {
			if (Header_ == p) {//如果是头结点
				Header_ = p->next_;
			}
			if (p->pre_) {
				p->pre_->next_ = p->next_;
			}
			if (p->next_) {
				p->next_->pre_ = p->pre_;
			}
			p->pre_ = 0;
			p->next_ = 0;
			Allocator_->Free(p->node_);
			--Size_;
			return true;
		}
		return false;
	}

	inline T * Get(const KEY& key) {
		LISTNODE * p = GetListNode(key);
		if (p) {
			return &p->v_;
		}
		return 0;
	}

	inline LISTNODE * GetFirstNode() {
		return Header_;
	}

	inline unsigned int Size() const {
		return Size_;
	}

private:
	inline static bool DefPred(const T &) {
		return true;
	}
	
	inline LISTNODE * GetListNode(const KEY& key) {
		if (Header_) {
			LISTNODE * p = Header_;
			while (p) {
				if (p->key_ == key) { //如果已经存在
					return p;
				}
				p = p->next_;
			}
		}
		return 0;
	}

private:
	LISTNODE * Header_; //头指针
	Allocator * Allocator_;
	unsigned int Size_;
};

template <typename DATA, typename KEY = UintHashProxy, typename ALLOCATOR = PoolNodeAllocator<PoolNode<ListNode<DATA, KEY> > > >
class FastList //二维列表
{
#ifdef _MSC_VER
	typedef unsigned __int64 UINT64;
#else
	typedef unsigned long long UINT64;
#endif
public:
	typedef FastListLine<DATA, KEY, ALLOCATOR> LINE;
	typedef ListNode<DATA, KEY> LISTNODE;
	typedef ALLOCATOR allocator_type;
	typedef PoolNode<LISTNODE> POOLNODE;

	FastList(unsigned int size, unsigned int start, unsigned int end, unsigned int line) //[start, end], line不能为0
		: Allocator_(new ALLOCATOR(size)), SelfAlloc_(true),
		Start_(start), End_(end), Line_(line), 
		Perline_(((UINT64)end - start + 1) / line + (((UINT64)end - start + 1) % line ? 1 : 0)),
		Lines_(Line_, LINE(*Allocator_)),
		Size_(0) {
	}

	FastList(unsigned int start, unsigned int end, unsigned int line, ALLOCATOR & allocator) //[start, end], line不能为0
		: Allocator_(&allocator), SelfAlloc_(false),
		Start_(start), End_(end), Line_(line), 
		Perline_(((UINT64)end - start + 1) / line + (((UINT64)end - start + 1) % line ? 1 : 0)),
		Lines_(Line_, LINE(*Allocator_)),
		Size_(0) {
	}

	~FastList() {
		if (SelfAlloc_) {
			delete Allocator_;
		}
	}

	inline DATA * Add(const KEY& key, const DATA& data) {
		LINE * line = GetLine(key);
		if (line) {
			bool add = false;
			DATA * p = line->Add(key, data, add);
			if (p && add) ++Size_;
			return p;
		}
		return 0;
	}

	inline bool Del(const KEY& key) {
		LINE * line = GetLine(key);
		if (line) {
			bool ret = line->Del(key);
			if (ret) --Size_;
			return ret;
		}
		return false;
	}

	template <typename PRED>
	inline bool DelIf(const KEY& key, PRED pred) {
		LINE * line = GetLine(key);
		if (line) {
			bool ret = line->DelIf(key, pred);
			if (ret) --Size_;
			return ret;
		}
		return false;
	}

	inline DATA * Get(const KEY& key) {
		LINE * line = GetLine(key);
		if (line) {
			return line->Get(key);
		}
		return 0;
	}

	inline unsigned int Size() const {
		return Size_;
	}

	inline LISTNODE * GetFirstNode() {
		for (unsigned int i = 0; i < Line_; ++i) {
			if (Lines_[i].Capacity() > 0) {
				return Lines_[i].GetFirstNode();
			}
		}
		return 0;
	}

	inline LISTNODE * GetLineFirstNode(unsigned int line) {
		if (line < Line_) return Lines_[line].GetFirstNode();
		else return 0;
	}

	inline unsigned int TotalLine() const {
		return Line_;
	}

	inline POOLNODE * GetFirstPoolNode() {
		return Allocator_->FirstUsedNode();
	}

private:
	inline LINE * GetLine(const KEY& key) {
		unsigned int pos = key.Hash();
		if (pos >= Start_ && pos <= End_) {
			unsigned int line = (pos - Start_) / Perline_;
			return &Lines_[line];
		}
		return 0;
	}

private:
	ALLOCATOR * Allocator_;
	bool SelfAlloc_;
	const unsigned int Start_;
	const unsigned int End_;		
	const unsigned int Line_; //行数
	const UINT64 Perline_; //每行的元素个数
	std::vector<LINE> Lines_; //行数组
	unsigned int Size_;
}; //end class FastList
}; //end namespace fund

#endif
