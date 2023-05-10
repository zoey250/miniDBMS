#include "restrictinfo.h"
#include "nodeFuncs.h"
#include "optimizer.h"

//
// Created by elias on 23-5-8.
//
RestrictInfo *
make_restrictinfo(PlannerInfo *root,
                Expr *clause,
                Relids required_relids)
{
    RestrictInfo *restrictinfo = makeNode(RestrictInfo);

    restrictinfo->clause = clause;

    if (list_length(((OpExpr *) clause)->args) == 2)
    {
        restrictinfo->left_relids = pull_varnos(root, get_leftop(clause));
        restrictinfo->right_relids = pull_varnos(root, get_rightop(clause));

        restrictinfo->clause_relids = bms_union(restrictinfo->left_relids,
                                                restrictinfo->right_relids);

        if (!bms_is_empty(restrictinfo->left_relids) &&
            !bms_is_empty(restrictinfo->right_relids) &&
            !bms_overlap(restrictinfo->left_relids,
                         restrictinfo->right_relids))
        {
            restrictinfo->can_join = true;
        }
    }
    // TODO else

    if (required_relids != NULL)
    {
        restrictinfo->required_relids = required_relids;
    }
    else
    {
        restrictinfo->required_relids = restrictinfo->clause_relids;
    }

    return restrictinfo;
}
