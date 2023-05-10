//
// Created by elias on 23-5-7.
//
#include "list.h"
#include "nodes.h"

void *
copyObjectImpl(const void *from)
{
    void   *retval;

    if (from == NULL)
    {
        return NULL;
    }

    switch (nodeTag(from))
    {
        case T_List:
//            retval = list_copy_deep(from);
            break;
            // TODO finish
        case T_OpExpr:
        case T_Var:
        default:
            retval = 0;
            break;
    }

    return retval;
}
