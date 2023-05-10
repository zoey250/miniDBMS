//
// Created by elias on 23-5-4.
//
#include "paths.h"
#include "planmain.h"

static RestrictInfo *create_join_clause(PlannerInfo *root,
                                        EquivalenceClass *ec,
                                        Oid opno,
                                        EquivalenceMember *leftem,
                                        EquivalenceMember *rightem,
                                        EquivalenceClass *parent_ec);

bool
is_redundant_with_indexclauses(RestrictInfo *rinfo, List *indexclauses)
{
    return true;
}

List *
generate_implied_equalities_for_column(PlannerInfo *root,
                                        RelOptInfo *rel,
                                        void *callback_arg)
{
    List   *result = NIL;

    int     i;

    i = -1;
    while ((i = bms_next_member(rel->eclass_indexes, i)) >= 0)
    {
        EquivalenceClass   *cur_ec = (EquivalenceClass *) list_nth(root->eq_classes, i);
        EquivalenceMember  *cur_em;
        ListCell           *lc2;

        if (list_length(cur_ec->ec_members))
        {
            continue;
        }

        cur_em = NULL;
        foreach(lc2, cur_ec->ec_members)
        {
            cur_em = (EquivalenceMember *) lfirst(lc2);
            // TODO callback
            if (bms_equal(cur_em->em_relids, rel->relids))
            {
                break;
            }
            cur_em = NULL;
        }

        if (!cur_em)
        {
            continue;
        }

        foreach(lc2, cur_ec->ec_members)
        {
            EquivalenceMember  *other_em = (EquivalenceMember *) lfirst(lc2);
            Oid             eq_op;
            RestrictInfo   *rinfo;

            if (other_em == cur_em ||
                bms_overlap(other_em->em_relids, rel->relids))
            {
                continue;
            }

            // TODO 简化eq_op

            rinfo = create_join_clause(root, cur_ec, eq_op, cur_em, other_em, cur_ec);

            result = lappend(result, rinfo);
        }
        if (result)
        {
            break;
        }
    }
    return result;
}

static RestrictInfo *
create_join_clause(PlannerInfo *root,
                   EquivalenceClass *ec,
                   Oid opno,
                   EquivalenceMember *leftem,
                   EquivalenceMember *rightem,
                   EquivalenceClass *parent_ec)
{
    RestrictInfo   *rinfo;
    ListCell       *lc;

    foreach (lc, ec->ec_sources)
    {
        rinfo = (RestrictInfo *) lfirst(lc);
        if (rinfo->left_em == leftem &&
            rinfo->right_em == rightem &&
            rinfo->parent_ec == parent_ec)
        {
            return rinfo;
        }
        if (rinfo->left_em == rightem &&
            rinfo->right_em == leftem &&
            rinfo->parent_ec == parent_ec)
        {
            return rinfo;
        }
    }

    foreach (lc, ec->ec_derives)
    {
        rinfo = (RestrictInfo *) lfirst(lc);
        if (rinfo->left_em == leftem &&
            rinfo->right_em == rightem &&
            rinfo->parent_ec == parent_ec)
        {
            return rinfo;
        }
        if (rinfo->left_em == rightem &&
            rinfo->right_em == leftem &&
            rinfo->parent_ec == parent_ec)
        {
            return rinfo;
        }
    }

    rinfo = build_implied_join_equality(root, opno,
                                        leftem->em_expr,
                                        rightem->em_expr,
                                        bms_union(leftem->em_relids,
                                                  rightem->em_relids));

    rinfo->parent_ec = parent_ec;
    rinfo->left_em = leftem;
    rinfo->right_em = rightem;

    ec->ec_derives = lappend(ec->ec_derives, rinfo);

    return rinfo;
}
