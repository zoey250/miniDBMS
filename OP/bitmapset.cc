//
// Created by elias on 23-5-2.
//

#include <stddef.h>
#include "bitmapset.h"
#include "bitutils.h"

#define WORDNUM(x)  ((x) / BITS_PER_BITMAPWORD)
#define BITNUM(x)   ((x) % BITS_PER_BITMAPWORD)

#define BITMAPSET_SIZE(nwords) \
    (offsetof(Bitmapset, words) + (nwords) * sizeof(bitmapword))

#define bmw_rightmost_one_pos(w)    rightmost_one_pos64(w);

BMS_Comparison
bms_subset_compare(const Bitmapset *a, const Bitmapset *b)
{
    BMS_Comparison  result;
    int             shortlen;
    int             longlen;
    int             i;

    if (a == NULL)
    {
        if (b == NULL)
        {
            return BMS_EQUAL;
        }
        return bms_is_empty(b) ? BMS_EQUAL : BMS_SUBSET1;
    }
    if (b == NULL)
    {
        return bms_is_empty(a) ? BMS_EQUAL : BMS_SUBSET2;
    }

    result = BMS_EQUAL;
    shortlen = Min(a->nwords, b->nwords);

    for (i = 0; i < shortlen; ++i)
    {
        bitmapword  aword = a->words[i];
        bitmapword  bword = b->words[i];

        if ((aword & ~bword) != 0)
        {
            if (result == BMS_SUBSET1)
            {
                return BMS_DIFFERENT;
            }
            result = BMS_SUBSET2;
        }

        if ((bword & ~aword) != 0)
        {
            if (result == BMS_SUBSET2)
            {
                return BMS_DIFFERENT;
            }
            result = BMS_SUBSET1;
        }
    }

    if (a->nwords > b->nwords)
    {
        longlen = a->nwords;
        for (; i < longlen; ++i)
        {
            if (a->words[i] != 0)
            {
                if (result == BMS_SUBSET1)
                {
                    return BMS_DIFFERENT;
                }
                result = BMS_SUBSET2;
            }
        }
    }
    else if (a->nwords < b->nwords)
    {
        longlen = b->nwords;
        for (; i < longlen; ++i)
        {
            if (b->words[i] != 0)
            {
                if (result == BMS_SUBSET2)
                {
                    return BMS_DIFFERENT;
                }
                result = BMS_SUBSET1;
            }
        }
    }
    return result;
}

bool
bms_is_empty(const Bitmapset *a)
{
    int nwords;
    int wordnum;

    if (a == NULL) {
        return true;
    }

    nwords = a->nwords;

    for (wordnum = 0; wordnum < nwords; ++wordnum)
    {
        bitmapword w = a->words[wordnum];

        if (w != 0)
        {
            return false;
        }
    }
    return true;
}

bool
bms_is_member(int x, const Bitmapset *a)
{
    int     wordnum;
    int     bitnum;

    if (a == NULL)
    {
        return false;
    }

    wordnum = WORDNUM(x);
    bitnum = BITNUM(x);

    if (wordnum >= a->nwords)
    {
        return false;
    }
    if ((a->words[wordnum] & ((bitmapword) 1 << bitnum)) != 0)
    {
        return true;
    }
    return false;
}

Bitmapset *
bms_add_members(Bitmapset *a, const Bitmapset *b)
{
    Bitmapset  *result;
    const Bitmapset *other;
    int         otherlen;
    int         i;

    if (a == NULL)
    {
        return bms_copy(b);
    }
    if (b == NULL)
    {
        return a;
    }

    if (a->nwords < b->nwords)
    {
        result = bms_copy(b);
        other = a;
    }
    else
    {
        result = a;
        other = b;
    }

    otherlen = other->nwords;

    for (i = 0; i < otherlen; ++i)
    {
        result->words[i] |= other->words[i];
    }
    if (result != a)
    {
        free(a);
    }
    return result;
}

Bitmapset *
bms_copy(const Bitmapset *a)
{
    Bitmapset  *result;
    size_t      size;

    if (a == NULL)
    {
        return NULL;
    }

    size = BITMAPSET_SIZE(a->nwords);

    result = (Bitmapset *) malloc(size);
    memcpy(result, a, size);
    return result;
}

Bitmapset *
bms_del_member(Bitmapset *a, int x)
{
    int     wordnum;
    int     bitnum;

    if (a == NULL)
    {
        return NULL;
    }

    wordnum = WORDNUM(x);
    bitnum = BITNUM(x);

    if (wordnum < a->nwords)
    {
        a->words[wordnum] &= ~((bitmapword) 1 << bitnum);
    }
    return a;
}

int
bms_next_member(const Bitmapset *a, int prevbit)
{
    int         nwords;
    int         wordnum;
    bitmapword  mask;

    if (a == NULL)
    {
        return -2;
    }

    nwords = a->nwords;
    prevbit++;
    mask = (~(bitmapword) 0) << BITNUM(prevbit);

    for (wordnum = WORDNUM(prevbit); wordnum < nwords; wordnum++)
    {
        bitmapword  w = a->words[wordnum];

        w &= mask;

        if (w != 0)
        {
            int     result;

            result = wordnum * BITS_PER_BITMAPWORD;
            result += bmw_rightmost_one_pos(w);
            return result;
        }

        mask = (~(bitmapword) 0);
    }
    return -2;
}

bool
bms_equal(const Bitmapset *a, const Bitmapset *b)
{
    const Bitmapset    *shorter;
    const Bitmapset    *longer;
    int     shortlen;
    int     longlen;
    int     i;

    if (a == NULL)
    {
        if (b == NULL)
        {
            return true;
        }
        return bms_is_empty(b);
    }
    else if (b == NULL)
    {
        return bms_is_empty(a);
    }

    if (a->nwords <= b->nwords)
    {
        shorter = a;
        longer = b;
    }
    else
    {
        shorter = b;
        longer = a;
    }

    shortlen =  shorter->nwords;
    for (i = 0; i < shortlen; ++i)
    {
        if (shorter->words[i] != longer->words[i])
        {
            return false;
        }
    }

    longlen = longer->nwords;

    for (; i < longlen; ++i)
    {
        if (longer->words[i] != 0)
        {
            return false;
        }
    }
    return true;
}

bool
bms_overlap(const Bitmapset *a, const Bitmapset *b)
{
    int     shortlen;
    int     i;

    if (a == NULL || b == NULL)
    {
        return false;
    }

    shortlen = Min(a->nwords, b->nwords);

    for (int i = 0; i < shortlen; ++i)
    {
        if ((a->words[i] & b->words[i]) != 0)
        {
            return true;
        }
    }
    return false;
}

Bitmapset *
bms_add_member(Bitmapset *a, int x)
{
    int     wordnum;
    int     bitnum;

    if (a == NULL)
    {
        return bms_make_singleton(x);
    }

    wordnum = WORDNUM(x);
    bitnum = BITNUM(x);

    if (wordnum >= a->nwords)
    {
        int     oldnwords = a->nwords;
        int     i;

        a = (Bitmapset *) realloc(a, BITMAPSET_SIZE(wordnum + 1));
        a->nwords = wordnum + 1;

        for (i = oldnwords; i < a->nwords; ++i)
        {
            a->words[i] = 0;
        }
    }

    a->words[wordnum] |= ((bitmapword) 1 << bitnum);
    return a;
}

Bitmapset *
bms_make_singleton(int x)
{
    Bitmapset  *result;
    int         wordnum;
    int         bitnum;

    wordnum = WORDNUM(x);
    bitnum = BITNUM(x);
    result = (Bitmapset *) malloc(BITMAPSET_SIZE(wordnum + 1));
    memset(result, 0, BITMAPSET_SIZE(wordnum + 1));

    result->type = T_Bitmapset;
    result->nwords = wordnum + 1;
    result->words[wordnum] = ((bitmapword) 1 << bitnum);
    return result;
}

Bitmapset *
bms_union(const Bitmapset *a, const Bitmapset *b)
{
    Bitmapset  *result;
    const Bitmapset *other;
    int         otherlen;
    int         i;

    if (a == NULL)
    {
        return bms_copy(b);
    }
    if (b == NULL)
    {
        return bms_copy(a);
    }

    if (a->nwords <= b->nwords)
    {
        result = bms_copy(b);
        other = a;
    }
    else
    {
        result = bms_copy(a);
        other = b;
    }

    otherlen = other->nwords;
    for (i = 0; i < otherlen; ++i)
    {
        result->words[i] |= other->words[i];
    }
    return result;
}