
#ifndef _COMM_SOCKCOMMU_CACHE_H_
#define _COMM_SOCKCOMMU_CACHE_H_

#include <sys/time.h>
#include "mempool.h"


//纯cache对象
class CRawCache
{
public:
    CRawCache(CMemPool& mp);
    ~CRawCache();

    char* data();
    unsigned data_len();
    void append(const char* data, unsigned data_len);
    void skip(unsigned length);

private:
    //内存池对象
    CMemPool& _mp;
    //内存基址
    char* _mem;
    //内存大小
    unsigned _block_size;
    //实际使用内存起始偏移量
    unsigned _data_head;
    //实际使用内存长度
    unsigned _data_len;
};

#endif

