//
// Created by elias on 23-4-30.
//

#ifndef MICRODBMS_LIST_H
#define MICRODBMS_LIST_H

#include "c.h"
#include "nodes.h"

#define NIL ((List *) NULL)

typedef union ListCell
{
    void   *ptr_value;
    int     int_value;
} ListCell;

typedef struct List
{
    NodeTag     type;
    int         length;
    int         max_length;
    ListCell   *elements;
    ListCell    initial_elements[FLEXIBLE_ARRAY_MEMBER];
} List;

typedef struct ForEachState
{
    const List *l;
    int         i;
} ForEachState;

typedef struct ForBothState
{
    const List *l1;
    const List *l2;
    int i;
} ForBothState;

#define foreach(cell, lst) \
    for (ForEachState cell##__state = {(lst), 0}; \
         (cell##__state.l != NIL && \
          cell##__state.i < cell##__state.l->length) ? \
         (cell = &cell##__state.l->elements[cell##__state.i], true) : \
         (cell = NULL, false); \
         cell##__state.i++)

#define forboth(cell1, list1, cell2, list2) \
    for (ForBothState cell1##__state = {(list1), (list2), 0}; \
         multi_for_advance_cell(cell1, cell1##__state, l1, i), \
         multi_for_advance_cell(cell2, cell1##__state, l2, i), \
         (cell1 != NULL && cell2 != NULL); \
         cell1##__state.i++)

#define multi_for_advance_cell(cell, state, l, i) \
    (cell = (state.l != NIL && state.i < state.l->length) ? \
     &state.l->elements[state.i] : NULL)

static inline ListCell *
list_last_cell(const List *list)
{
    return &list->elements[list->length - 1];
}

static inline ListCell *
list_nth_cell(const List *list, int n)
{
    return &list->elements[n];
}

static inline int
list_length(const List *l)
{
    return l ? l->length : 0;
}

#define lfirst(lc)              ((lc)->ptr_value)
#define lfirst_node(type, lc)   castNode(type, lfirst(lc))

#define linitial(l)     lfirst(list_nth_cell(l, 0))

#define lsecond(l)      lfirst(list_nth_cell(l, 1))

#define llast(l)        lfirst(list_last_cell(l))

extern List *list_make1_impl(NodeTag t, ListCell datum1);
extern List *list_make2_impl(NodeTag t, ListCell datum1, ListCell datum2);

extern List *list_insert_nth(List *list, int pos, void *datum);

extern List *lappend(List *list, void *datum);

extern List *lcons(void *datum, List *list);

extern List *list_copy_deep(const List *oldlist);

#define list_make_ptr_cell(v) ((ListCell) {.ptr_value = (v)})

#define list_make1(x1) \
    list_make1_impl(T_List, list_make_ptr_cell(x1))
#define list_make2(x1, x2) \
    list_make2_impl(T_List, list_make_ptr_cell(x1), list_make_ptr_cell(x2))

#define foreach_current_index(cell) (cell##__state.i)

static inline void *
list_nth(const List *list, int n)
{
    return lfirst(list_nth_cell(list, n));
}

#endif //MICRODBMS_LIST_H
