//
// Created by elias on 23-5-8.
//

#ifndef MICRODBMS_NODEFUNCS_H
#define MICRODBMS_NODEFUNCS_H

#include "nodes.h"
#include "primnodes.h"

static inline Node *
get_leftop(const void *clause) {
    const OpExpr *expr = (const OpExpr *) clause;
    if (expr->args != NIL)
    {
        return (Node *) linitial(expr->args);
    }
    else
    {
        return NULL;
    }
}

static inline Node *
get_rightop(const void *clause)
{
    const OpExpr *expr = (const OpExpr *) clause;

    if (list_length(expr->args) >= 2)
    {
        return (Node *) lsecond(expr->args);
    }
    else
    {
        return NULL;
    }
}

#endif //MICRODBMS_NODEFUNCS_H
