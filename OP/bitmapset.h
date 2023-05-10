//
// Created by elias on 23-5-2.
//

#ifndef MICRODBMS_BITMAPSET_H
#define MICRODBMS_BITMAPSET_H

#include "c.h"
#include "nodes.h"

typedef enum
{
    BMS_EQUAL,
    BMS_SUBSET1,
    BMS_SUBSET2,
    BMS_DIFFERENT
} BMS_Comparison;

#define BITS_PER_BITMAPWORD 64

typedef uint64 bitmapword;

typedef struct Bitmapset
{
    NodeTag     type;
    int         nwords;
    bitmapword  words[FLEXIBLE_ARRAY_MEMBER];
} Bitmapset;

extern BMS_Comparison bms_subset_compare(const Bitmapset *a, const Bitmapset *b);
extern bool bms_is_empty(const Bitmapset *a);
extern bool bms_is_member(int x, const Bitmapset *a);
extern Bitmapset *bms_add_members(Bitmapset *a, const Bitmapset *b);
extern Bitmapset *bms_copy(const Bitmapset *a);
extern Bitmapset *bms_del_member(Bitmapset *a, int x);
extern int bms_next_member(const Bitmapset *a, int prevbit);
extern bool bms_equal(const Bitmapset *a, const Bitmapset *b);
extern bool bms_overlap(const Bitmapset *a, const Bitmapset *b);

extern Bitmapset *bms_add_member(Bitmapset *a, int x);
extern Bitmapset *bms_make_singleton(int x);
extern Bitmapset *bms_union(const Bitmapset *a, const Bitmapset *b);

#endif //MICRODBMS_BITMAPSET_H
