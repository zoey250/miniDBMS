//
// Created by elias on 23-5-1.
//

#include "list.h"
#include "bitutils.h"

static List *new_list(NodeTag type, int min_size);
static ListCell *insert_new_cell(List *list, int pos);
static void enlarge_list(List *list, int min_size);
static void new_tail_cell(List *list);
static void new_head_cell(List *list);

#define LIST_HEADER_OVERHEAD \
    ((int) (offsetof(List, initial_elements) - 1) / sizeof(ListCell) + 1)

List *
list_insert_nth(List *list, int pos, void *datum)
{
    if (list == NIL) {
        return list_make1(datum);
    }
    lfirst(insert_new_cell(list, pos)) = datum;
    return list;
}

List *
list_make1_impl(NodeTag t, ListCell datum1)
{
    List *list = new_list(t, 1);
    list->elements[0] = datum1;
    return list;
}

List *
list_make2_impl(NodeTag t, ListCell datum1, ListCell datum2)
{
    List   *list = new_list(t, 2);
    list->elements[0] = datum1;
    list->elements[1] = datum2;
    return list;
}

List *
lappend(List *list, void *datum)
{
    if (list == NIL)
    {
        list = new_list(T_List, 1);
    }
    else
    {
        new_tail_cell(list);
    }
    llast(list) = datum;
    return list;
}

List *
lcons(void *datum, List *list)
{
    if (list == NIL)
    {
        list = new_list(T_List, 1);
    }
    else
    {
        new_head_cell(list);
    }
    linitial(list) = datum;
    return list;
}

List *
list_copy_deep(const List *oldlist)
{
    List   *newlist;

    if (oldlist == NIL)
    {
        return NIL;
    }

    newlist = new_list(oldlist->type, oldlist->length);
    for (int i = 0; i < newlist->length; ++i)
    {
        lfirst(&newlist->elements[i]) =
                copyObjectImpl(lfirst(&oldlist->elements[i]));
    }
    return newlist;
}

static List *
new_list(NodeTag type, int min_size)
{
    List   *newlist;
    int     max_size;
    max_size = nextpower2_32(Max(8, min_size + LIST_HEADER_OVERHEAD));
    max_size -= LIST_HEADER_OVERHEAD;

    newlist = (List *) malloc(offsetof(List, initial_elements) +
                              max_size * sizeof(ListCell));
    newlist->type = type;
    newlist->length = min_size;
    newlist->elements = newlist->initial_elements;

    return newlist;
}

static ListCell *
insert_new_cell(List *list, int pos)
{
    if (list->length >= list->max_length)
    {
        enlarge_list(list, list->length + 1);
    }
    if (pos < list->length)
    {
        memmove(&list->elements[pos + 1], &list->elements[pos],
                (list->length - pos) * sizeof(ListCell));
    }
    list->length++;
    return &list->elements[pos];
}

static void
enlarge_list(List *list, int min_size)
{
    int     new_max_len;

    new_max_len = nextpower2_32(Max(16, min_size));

    if (list->elements == list->initial_elements)
    {
        list->elements = (ListCell *) malloc(new_max_len * sizeof(ListCell));
        memcpy(list->elements, list->initial_elements,
               list->length * sizeof(ListCell));
    }
    else
    {
        ListCell   *newelements;
        newelements = (ListCell *) malloc(new_max_len * sizeof(ListCell));
        memcpy(newelements, list->elements,
               list->length * sizeof(ListCell));
        free(list->elements);
        list->elements = newelements;
    }
    list->max_length = new_max_len;
}

static void
new_tail_cell(List *list)
{
    if (list->length >= list->max_length)
    {
        enlarge_list(list, list->length + 1);
    }
    list->length++;
}

static void
new_head_cell(List *list)
{
    if (list->length >= list->max_length)
    {
        enlarge_list(list, list->length + 1);
    }
    memmove(&list->elements[1], &list->elements[0],
            list->length * sizeof(ListCell));
    list->length++;
}