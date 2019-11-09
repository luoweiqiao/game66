#include <string.h>
#include <assert.h>
#include "cache.h"

CRawCache::CRawCache(CMemPool& mp)
: _mp(mp), _mem(NULL), _block_size(0), _data_head(0), _data_len(0)
{
}

CRawCache::~CRawCache(){}

char* CRawCache::data()
{
    if (_data_len == 0)
        return NULL;

    assert(_data_head < _block_size);
    return _mem + _data_head;
}

unsigned CRawCache::data_len(){return _data_len;}

void CRawCache::append(const char* data, unsigned data_len)
{
    //assert(data_len < 0x00010000);

    if (_mem == NULL)
    {
        _mem = (char*) _mp.allocate(data_len, _block_size);
        //assert(_mem);
        if(!_mem)
            throw(0);

        memcpy(_mem, data, data_len);

        _data_head = 0;
        _data_len = data_len;
        return;
    }
    
    //
    //	data_len < _block_size - _data_head - _data_len
    //	append directly
    //
    if (data_len + _data_head + _data_len <= _block_size)
    {
        memcpy(_mem + _data_head + _data_len, data, data_len);
        _data_len += data_len;
    }
    //
    //	_block_size-_data_len <= data_len
    //	reallocate new block. memmove, recycle
    //
    else if (data_len + _data_len > _block_size)
    {
        unsigned new_block_size = 0;
        char* mem = (char*) _mp.allocate(data_len+_data_len, new_block_size);
        //assert(mem);
        if(!mem)
        throw(0);

        memcpy(mem, _mem + _data_head, _data_len);
        memcpy(mem + _data_len, data, data_len);
        _mp.recycle(_mem, _block_size);

        _mem = mem;
        _block_size = new_block_size;
        _data_head = 0;
        _data_len += data_len;
    }
    //
    //	_block_size - _data_head - _data_len < data_len < _block_size-_datalen
    //	memmove data to block head, append new data
    //
    else
    //if ((data_len + _data_head + _data_len > _block_size) && (data_len + _data_len < _block_size))
    {
        memmove(_mem, _mem+_data_head, _data_len);
        memcpy(_mem+_data_len, data, data_len);

        _data_head = 0;
        _data_len += data_len;
    }
}
void CRawCache::skip(unsigned length)
{
    //	empty data
    if (_mem == NULL)
        return;

    //	not enough data
    else if (length >= _data_len)
    {
        _mp.recycle(_mem, _block_size);
        _mem = NULL;
        _block_size = _data_head = _data_len = 0;
        _data_head = 0;
        _data_len = 0;
    }

    //	skip part of data
    else
    {
        _data_head += length;
        _data_len -= length;
    }
}
