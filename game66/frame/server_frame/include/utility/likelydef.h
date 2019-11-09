
#ifndef _COMM_LIKELYDEF_H_
#define _COMM_LIKELYDEF_H_

#if !__GLIBC_PREREQ(2, 3)	
#define __builtin_expect(x, expected_value) (x)
#endif

#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif


#endif

