//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_BITUTILS_H
#define MICRODBMS_BITUTILS_H

#include "c.h"

//extern const uint8 leftmost_one_pos[256];

static inline int
leftmost_one_pos32(uint32 word)
{
    return 31 - __builtin_clz(word);
    /*
    int     shift = 32 - 8;
    while ((word >> shift) == 0)
    {
        shift -= 8;
    }
    return shift + leftmost_one_pos[(word >> shift) & 255];
    */
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
