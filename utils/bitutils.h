//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_BITUTILS_H
#define MICRODBMS_BITUTILS_H

#include "c.h"

static inline int
leftmost_one_pos32(uint32 word)
{
    return 31 - __builtin_clz(word);
}

static inline uint32
nextpower2_32(uint32 num)
{
    if ((num & (num - 1)) == 0)
    {
        return num;
    }

    return ((uint32) 1) << (leftmost_one_pos32(num) + 1);
}

static inline int
rightmost_one_pos64(uint64 word)
{
    // TODO #ifdef HAVE__BUILTIN_CTZ
    return __builtin_ctzl(word);
}

#endif //MICRODBMS_BITUTILS_H
