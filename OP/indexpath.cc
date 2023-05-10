//
// Created by elias on 23-5-3.
//

#include "pathnodes.h"
#include "indexpath.h"
#include "paths.h"

typedef struct
{
    IndexOptInfo   *index;
    int             indexcol;
} ec_member_matches_arg;

static void match_restriction_clauses_to_index(PlannerInfo *root,
                                               IndexOptInfo *index,
                                               IndexClauseSet *clauseset);
static void match_clauses_to_index(PlannerInfo *root,
                                   List *clauses,
                                   IndexOptInfo *index,
                                   IndexClauseSet *clauseset);
static void match_clause_to_index(PlannerInfo *root,
                                  RestrictInfo *rinfo,
                                  IndexOptInfo *index,
                                  IndexClauseSet *clauseset);
static IndexClause *match_clause_to_indexcol(PlannerInfo *root,
                                          RestrictInfo *rinfo,
                                          int indexcol,
                                          IndexOptInfo *index);
static IndexClause *match_opclause_to_indexcol(PlannerInfo *root,
                                               RestrictInfo *rinfo,
                                               int indexcol,
                                               IndexOptInfo *index);
static void get_index_paths(PlannerInfo *root, RelOptInfo *rel,
                            IndexOptInfo *index, IndexClauseSet *clauses);
static List *build_index_paths(PlannerInfo *root, RelOptInfo *rel,
                               IndexOptInfo *index, IndexClauseSet *clauses);
static double get_loop_count(PlannerInfo *root, Index cur_relid, Relids outer_relids);
static void match_eclass_clauses_to_index(PlannerInfo *root, IndexOptInfo *index,
                                          IndexClauseSet *clauseSet);
static bool ec_member_matches_indexcol();

void
create_index_paths(PlannerInfo *root, RelOptInfo *rel)
{
    List       *indexpaths;
    IndexClauseSet  rclauseset;
    IndexClauseSet  eclauseset;
    ListCell   *lc;

    if (rel->indexlist == NIL)
    {
        return;
    }

    foreach(lc, rel->indexlist)
    {
        IndexOptInfo   *index = (IndexOptInfo *) lfirst(lc);

        memset(&rclauseset, 0, sizeof(rclauseset));
        match_restriction_clauses_to_index(root, index, &rclauseset);

        get_index_paths(root, rel, index, &rclauseset);

        memset(&eclauseset, 0, sizeof(eclauseset));
        match_eclass_clauses_to_index(root, index, &eclauseset);

        // TODO consider_index_join_clauses
        if (eclauseset.nonempty)
        {
//            consider_index_join_clauses();
        }
    }
}

bool
match_index_to_operand(Node *operand,
                       int indexcol,
                       IndexOptInfo *index)
{
    int     indkey;

//    if (operand && IsA())

    indkey = index->indexkeys[indexcol];
    if (indkey != 0)
    {
        if (operand && IsA(operand, Var) &&
            index->rel->relid == ((Var *) operand)->varno &&
            indkey == ((Var *) operand)->varattno)
        {
            return true;
        }
    }
    return false;
}

static void
match_restriction_clauses_to_index(PlannerInfo *root,
                                   IndexOptInfo *index,
                                   IndexClauseSet *clauseset)
{
    match_clauses_to_index(root, index->indrestrictinfo, index, clauseset);
}

static void
match_clauses_to_index(PlannerInfo *root,
                       List *clauses,
                       IndexOptInfo *index,
                       IndexClauseSet *clauseset)
{
    ListCell   *lc;

    foreach(lc, clauses)
    {
        RestrictInfo   *rinfo = lfirst_node(RestrictInfo, lc);
        match_clause_to_index(root, rinfo, index, clauseset);
    }
}

static void
match_clause_to_index(PlannerInfo *root,
                      RestrictInfo *rinfo,
                      IndexOptInfo *index,
                      IndexClauseSet *clauseset)
{
    int     indexcol;

    for (indexcol = 0; indexcol < index->nkeycolumns; indexcol++)
    {
        IndexClause *iclause;
        ListCell       *lc;

        foreach(lc, clauseset->indexclauses[indexcol])
        {
            iclause = (IndexClause *) lfirst(lc);
            if (iclause->rinfo == rinfo)
            {
                return;
            }
        }

        iclause = match_clause_to_indexcol(root, rinfo, indexcol, index);

        if (iclause)
        {
            clauseset->indexclauses[indexcol] =
                lappend(clauseset->indexclauses[indexcol], iclause);
            clauseset->nonempty = true;
            return;
        }
    }
}

static IndexClause *
match_clause_to_indexcol(PlannerInfo *root,
                         RestrictInfo *rinfo,
                         int indexcol,
                         IndexOptInfo *index)
{
    Expr           *clause = rinfo->clause;

    if (clause == NULL)
    {
        return NULL;
    }

    if (IsA(clause, OpExpr))
    {
        return (match_opclause_to_indexcol(root, rinfo, indexcol, index));
    }
    return NULL;
}

static IndexClause *
match_opclause_to_indexcol(PlannerInfo *root,
                           RestrictInfo *rinfo,
                           int indexcol,
                           IndexOptInfo *index)
{
    IndexClause    *iclause;
    OpExpr         *clause = (OpExpr *) rinfo->clause;

    Node           *leftop;
    Node           *rightop;

    Oid             expr_op;
    Index           index_relid;

    if (list_length(clause->args) != 2)
    {
        return NULL;
    }

    leftop = (Node *) linitial(clause->args);
    rightop = (Node *) lsecond(clause->args);

    expr_op = clause->opno;

    index_relid = index->rel->relid;

    if (match_index_to_operand(leftop, indexcol, index) &&
        !bms_is_member(index_relid, rinfo->right_relids))
    {
        iclause = makeNode(IndexClause);
        iclause->rinfo = rinfo;
        iclause->indexquals = list_make1(rinfo);
        iclause->indexcol = indexcol;
        return iclause;
    }

    // TODO maybe unuse
    if (match_index_to_operand(rightop, indexcol, index) &&
        !bms_is_member(index_relid, rinfo->left_relids))
    {

    }
    return NULL;
}

static void
get_index_paths(PlannerInfo *root, RelOptInfo *rel,
                IndexOptInfo *index, IndexClauseSet *clauses)
{
    List       *indexpaths;
    ListCell   *lc;

    indexpaths = build_index_paths(root, rel, index, clauses);

    foreach(lc, indexpaths)
    {
        IndexPath  *ipath = (IndexPath *) lfirst(lc);

        add_path(rel, (Path *) ipath);
    }
}

static List *
build_index_paths(PlannerInfo *root, RelOptInfo *rel, IndexOptInfo *index, IndexClauseSet *clauses)
{
    List       *result = NIL;
    IndexPath  *ipath;
    List       *index_clauses;
    Relids      outer_relids;
    double      loop_count;

    int         indexcol;

    index_clauses = NIL;

    for (indexcol = 0; indexcol < index->nkeycolumns; indexcol++)
    {
        ListCell   *lc;

        foreach(lc, clauses->indexclauses[indexcol])
        {
            IndexClause    *iclause = (IndexClause *) lfirst(lc);
            RestrictInfo   *rinfo = iclause->rinfo;

            index_clauses = lappend(index_clauses, iclause);
            // TODO where set clause_relids?
            outer_relids = bms_add_members(outer_relids,
                                           rinfo->clause_relids);
        }
    }
    if (index_clauses == NIL)
    {
        return NIL;
    }

    outer_relids = bms_del_member(outer_relids, rel->relid);
    if (bms_is_empty(outer_relids))
    {
        outer_relids = NULL;
    }

    loop_count = get_loop_count(root, rel->relid, outer_relids);

    if (index_clauses != NIL)
    {
        // TODO useful_pathkeys
        List *useful_pathkeys = NULL;
        ipath = create_index_path(root, index, index_clauses, useful_pathkeys, outer_relids, loop_count);
        result = lappend(result, ipath);
    }

    return result;
}

static double
get_loop_count(PlannerInfo *root, Index cur_relid, Relids outer_relids)
{
    double  result;
    int     outer_relid;

    if (outer_relids == NULL)
    {
        return 1.0;
    }
}

static void
match_eclass_clauses_to_index(PlannerInfo *root, IndexOptInfo *index,
                              IndexClauseSet *clauseset)
{
    int     indexcol;

    if (!index->rel->has_eclass_joins)
    {
        return;
    }

    for (indexcol = 0; indexcol < index->nkeycolumns; indexcol++)
    {
        ec_member_matches_arg   arg;
        List   *clauses;

        arg.index = index;
        arg.indexcol = indexcol;

        clauses = generate_implied_equalities_for_column(root,
                                                         index->rel,
                                                         (void *) &arg);
        match_clauses_to_index(root, clauses, index, clauseset);
    }
}

// TODO 函数指针或extern
static bool
ec_member_matches_indexcol()
{

}