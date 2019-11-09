#ifndef _FUND_CONVERT_H_20060323
#define _FUND_CONVERT_H_20060323

#include <netinet/in.h>

namespace fund
{

  namespace Convert
  {
    template<typename T>
      inline void
      ToHostOrder(T& v)
      {
        v.ToHostOrder();
      }

    template<>
      inline void
      ToHostOrder<int8_t> (int8_t& v)
      {
        return;
      }

    template<>
      inline void
      ToHostOrder<uint8_t> (uint8_t& v)
      {
        return;
      }

    template<>
      inline void
      ToHostOrder<int16_t> (int16_t& v)
      {
        v = ntohs(v);
      }

    template<>
      inline void
      ToHostOrder<uint16_t> (uint16_t& v)
      {
        v = ntohs(v);
      }

    template<>
      inline void
      ToHostOrder<int32_t> (int32_t& v)
      {
        v = ntohl(v);
      }

    template<>
      inline void
      ToHostOrder<uint32_t> (uint32_t& v)
      {
        v = ntohl(v);
      }

    template<typename T>
      inline void
      ToNetOrder(T& v)
      {
        v.ToNetOrder();
      }

    template<>
      inline void
      ToNetOrder<int8_t> (int8_t& v)
      {
        return;
      }

    template<>
      inline void
      ToNetOrder<uint8_t> (uint8_t& v)
      {
        return;
      }

    template<>
      inline void
      ToNetOrder<int16_t> (int16_t& v)
      {
        v = htons(v);
      }

    template<>
      inline void
      ToNetOrder<uint16_t> (uint16_t& v)
      {
        v = htons(v);
      }

    template<>
      inline void
      ToNetOrder<int32_t> (int32_t& v)
      {
        v = htonl(v);
      }

    template<>
      inline void
      ToNetOrder<uint32_t> (uint32_t& v)
      {
        v = htonl(v);
      }

    template<typename T>
      inline T
      ToHostOrderByVal(T v)
      {
        ToHostOrder(v);
        return v;
      }

    template<typename T>
      inline T
      ToNetOrderByVal(T v)
      {
        ToNetOrder(v);
        return v;
      }

  }
  ;
}
;

#endif

