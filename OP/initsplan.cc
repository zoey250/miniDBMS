//
// Created by elias on 23-5-7.
//

#include "planmain.h"
#include "makefuncs.h"
#include "restrictinfo.h"

RestrictInfo *
build_implied_join_equality(PlannerInfo *root,
                            Oid opno,
                            Expr *item1,
                            Expr *item2,
                            Relids qualscope)
{
    RestrictInfo   *restrictinfo;
    Expr           *clause;

    clause = make_opclause(opno,
                           (Expr *) copyObject(item1),
                           (Expr *) copyObject(item2));

    restrictinfo = make_restrictinfo(root, clause, qualscope);
    return restrictinfo;
}