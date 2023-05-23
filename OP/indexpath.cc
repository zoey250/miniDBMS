//
// Created by elias on 23-5-3.
//

#include "pathnodes.h"
#include "indexpath.h"
#include "paths.h"
#include "restrictinfo.h"

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
                                          IndexOptInfo *index);
static IndexClause *match_opclause_to_indexcol(PlannerInfo *root,
                                               RestrictInfo *rinfo,
                                               IndexOptInfo *index);
static void get_index_paths(PlannerInfo *root, RelOptInfo *rel,
                            IndexOptInfo *index);
static List *build_index_paths(PlannerInfo *root, IndexOptInfo *index);
static double get_loop_count();

void
create_index_paths(PlannerInfo *root, RelOptInfo *rel)
{
//    IndexClauseSet  rclauseset;
    ListCell   *lc;

    if (rel->indexlist == NIL)
    {
        return;
    }

    foreach(lc, rel->indexlist)
    {
        IndexOptInfo   *index = (IndexOptInfo *) lfirst(lc);

//        memset(&rclauseset, 0, sizeof(rclauseset));
//        match_restriction_clauses_to_index(root, index, &rclauseset);
//
        get_index_paths(root, rel, index);
    }
}

bool
match_index_to_operand(Node *operand,
                       IndexOptInfo *index)
{
    int     indkey;

    indkey = index->indexkey;
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
    IndexClause *iclause;

    iclause = match_clause_to_indexcol(root, rinfo, index);

    if (iclause)
    {
        clauseset->indexclauses =
            lappend(clauseset->indexclauses, iclause);
        clauseset->nonempty = true;
        return;
    }
}

static IndexClause *
match_clause_to_indexcol(PlannerInfo *root,
                         RestrictInfo *rinfo,
                         IndexOptInfo *index)
{
    Expr           *clause = rinfo->clause;

    if (clause == NULL)
    {
        return NULL;
    }

    if (IsA(clause, OpExpr))
    {
        return match_opclause_to_indexcol(root, rinfo, index);
    }
    return NULL;
}

static IndexClause *
match_opclause_to_indexcol(PlannerInfo *root,
                           RestrictInfo *rinfo,
                           IndexOptInfo *index)
{
    IndexClause    *iclause;
    OpExpr         *clause = (OpExpr *) rinfo->clause;

    Node           *leftop;
    Node           *rightop;

    CompOp             expr_op;
    Index           index_relid;

    if (list_length(clause->args) != 2)
    {
        return NULL;
    }

    leftop = (Node *) linitial(clause->args);
    rightop = (Node *) lsecond(clause->args);

    expr_op = clause->opno;

    index_relid = index->rel->relid;

    if (match_index_to_operand(leftop, index) &&
        !bms_is_member(index_relid, rinfo->right_relids))
    {
        iclause = makeNode(IndexClause);
        iclause->rinfo = rinfo;
        iclause->indexquals = list_make1(rinfo);
        return iclause;
    }

    if (match_index_to_operand(rightop, index) &&
        !bms_is_member(index_relid, rinfo->left_relids))
    {
        CompOp     comm_op = get_commutator(expr_op);
        RestrictInfo   *commrinfo;

        commrinfo = commute_restrictinfo(rinfo, comm_op);
        iclause = makeNode(IndexClause);
        iclause->rinfo = rinfo;
        iclause->indexquals = list_make1(commrinfo);
        return iclause;
    }
    return NULL;
}

static void
get_index_paths(PlannerInfo *root, RelOptInfo *rel,
                IndexOptInfo *index)
{
    List       *indexpaths;
    ListCell   *lc;

    indexpaths = build_index_paths(root, index);

    foreach(lc, indexpaths)
    {
        IndexPath  *ipath = (IndexPath *) lfirst(lc);

        add_path(rel, (Path *) ipath);
    }
}

static List *
build_index_paths(PlannerInfo *root,
                  IndexOptInfo *index)
{
    List       *result = NIL;
    IndexPath  *ipath;
    double      loop_count;

    if (index->conditions.empty())
    {
        return NIL;
    }

    loop_count = get_loop_count();

    ipath = create_index_path(root, index, loop_count);
    result = lappend(result, ipath);

    return result;
}

static double
get_loop_count()
{
    return 1.0;
}
