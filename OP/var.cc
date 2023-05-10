//
// Created by elias on 23-5-8.
//

#include "planmain.h"

Relids
pull_varnos(PlannerInfo *root, Node *node)
{
    if (node == NULL)
    {
        return NULL;
    }

    // TODO maybe other type
    if (IsA(node, Var))
    {
        Var    *var = (Var *) node;

        return bms_add_member(NULL, var->varno);
    }

    return NULL;

}
