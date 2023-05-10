//
// Created by elias on 23-5-7.
//

#include "makefuncs.h"

Expr *
make_opclause(Oid opno, Expr *leftop, Expr *rightop)
{
    OpExpr *expr = makeNode(OpExpr);
    expr->opno = opno;

    if (rightop)
    {
        expr->args = list_make2(leftop, rightop);
    }
    else
    {
        expr->args = list_make1(leftop);
    }

    return (Expr *) expr;
}
