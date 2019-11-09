/*
 * RandomGenerator.h
 *
 *  Created on: 2012-4-17
 *      Author: toney
 */

#ifndef RANDOMGENERATOR_H_
#define RANDOMGENERATOR_H_
#include <cassert>
#include "fundamental/common.h"
namespace svrlib
{
class UInt64RandomGenerator
{
private:


private:
    void nextState()
    {
        uint64_t *p = state_;
        size_t j;

        left_ = NN;
        next_ = state_;

        for (j=NN-MM+1; --j; p++)
            *p = p[MM] ^ twist(p[0], p[1]);

        for (j=MM; --j; p++)
            *p = p[MM-NN] ^ twist(p[0], p[1]);

        *p = p[MM-NN] ^ twist(p[0], state_[0]);
    }

public:


    UInt64RandomGenerator(const uint8_t* pkeys, size_t keylen)
    {
        left_ = 1;
        next_ = NULL;
        if(keylen%sizeof(uint64_t) == 0)
        {
            init((uint64_t *)(pkeys), keylen/8);
        }
        else
        {
            size_t newkeylen = keylen + (sizeof(uint64_t) - keylen%sizeof(uint64_t));
            uint8_t* pnewkeys = new uint8_t[newkeylen];
            bzero(pnewkeys, newkeylen);
            memcpy(pnewkeys, pkeys, keylen);
            init((uint64_t *)(pnewkeys), newkeylen/8);
            delete[] pnewkeys;
        }
     }


    void init(uint64_t seed)
        {
            assert(sizeof(uint64_t)*8 == (size_t)INTTYPE_BITS);

            state_[0]= seed;
            for (size_t j=1; j<NN; j++)
            {
                state_[j]
                    = (INITVAL * (state_[j-1] ^ (state_[j-1] >> (INTTYPE_BITS-2)))
                    + (uint64_t)j);
            }
            left_ = 1;
        }

    void init(const uint64_t* initkeys, size_t keylen)
    {
        init(ARRAYINITVAL_0);

        size_t i = 1;
        size_t j = 0;
        size_t k = (NN > keylen ? NN : keylen);

        for (; k; k--)
        {
            state_[i]
                = (state_[i]
                ^ ((state_[i-1] ^ (state_[i-1] >> (INTTYPE_BITS-2)))
                * ARRAYINITVAL_1))
                + initkeys[j] + (uint64_t)j; /* non linear */

            i++;
            j++;

            if (i >= NN)
            {
                state_[0] = state_[NN-1];
                i = 1;
            }
            if (j >= keylen)
            {
                j = 0;
            }
        }

        for (k=NN-1; k; k--)
        {
            state_[i]
                = (state_[i]
                ^ ((state_[i-1] ^ (state_[i-1] >> (INTTYPE_BITS-2)))
                * ARRAYINITVAL_2))
                - (uint64_t)i; /* non linear */

            i++;

            if (i >= NN)
            {
                state_[0] = state_[NN-1];
                i = 1;
            }
        }

         /* MSB is 1; assuring non-zero initial array */
         state_[0] = (uint64_t)1 << (INTTYPE_BITS-1);
         left_ = 1;
    }

    /* generates a random number on [0,2^bits-1]-interval */
    uint64_t GetRandomNumber()
    {
        if (--left_ == 0) nextState();
        return temper(*next_++);
    }


private:

    static uint64_t twist(const uint64_t& u, const uint64_t& v)
       {
           static uint64_t mag01[2] = {0ULL, 0xB5026F5AA96619E9ULL};
           return ((((u & 0xFFFFFFFF80000000ULL) | (v & 0x7FFFFFFFULL)) >> 1) ^ mag01[v&1]);
       }

       static uint64_t temper(uint64_t y)
       {
           y ^= (y >> 29) & 0x5555555555555555ULL;
           y ^= (y << 17) & 0x71D67FFFEDA60000ULL;
           y ^= (y << 37) & 0xFFF7EEE000000000ULL;
           y ^= (y >> 43);

           return y;
       }

       static const int                INTTYPE_BITS = 64;
       static const size_t             NN = 312;
       static const size_t             MM = 156;
       static const uint64_t   ARRAYINITVAL_0 = 19650218ULL;
       static const uint64_t   ARRAYINITVAL_1 = 3935559000370003845ULL;
       static const uint64_t   ARRAYINITVAL_2 = 2862933555777941757ULL;
       static const uint64_t    INITVAL = 6364136223846793005ULL;


       // member variables
       uint64_t  state_[sizeof(uint64_t) *NN];
       size_t    left_;
       uint64_t* next_;


};
}
#endif /* RANDOMGENERATOR_H_ */
