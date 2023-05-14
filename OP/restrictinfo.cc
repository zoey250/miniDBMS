#include "restrictinfo.h"

//
// Created by elias on 23-5-8.
//

RestrictInfo *
commute_restrictinfo(RestrictInfo *rinfo, CompOp comm_op)
{
    RestrictInfo   *result;
    OpExpr         *newclause;
    OpExpr         *clause = castNode(OpExpr, rinfo->clause);

    newclause = makeNode(OpExpr);
    memcpy(newclause, clause, sizeof(OpExpr));

    newclause->opno = comm_op;
    newclause->args = list_make2(lsecond(clause->args),
                                 linitial(clause->args));

    result = makeNode(RestrictInfo);
    memcpy(result, rinfo, sizeof(RestrictInfo));

    result->clause = (Expr *) newclause;
    result->left_relids = rinfo->right_relids;
    result->right_relids = rinfo->left_relids;

    return result;
}

CompOp
get_commutator(CompOp opno)
{
    CompOp  result;
    switch (opno)
    {
        case LT_OP:
            result = GT_OP;
            break;
        case GT_OP:
            result = LT_OP;
            break;
        case LE_OP:
            result = GE_OP;
            break;
        case GE_OP:
            result = LE_OP;
            break;
        default:
            result = opno;
            break;
    }
    return result;
}