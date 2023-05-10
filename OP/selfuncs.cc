//
// Created by elias on 23-5-4.
//

#include <math.h>
#include "index_selfuncs.h"
#include "selfuncs.h"
#include "optimizer.h"

void
btcostestimate(struct PlannerInfo *root, struct IndexPath *path,
               double loop_count, Cost *indexStartupCost,
               Cost *indexTotalCost, Selectivity *indexSelectivity,
               double *indexPages)
{
    IndexOptInfo   *index = path->indexinfo;
    GenericCosts    costs = {0};
    Oid             relid;
    AttrNumber      colnum;
    double          numIndexTuples;
    Cost            desccentCost;
    List           *indexBoundQuals;
    int             indexcol;
    ListCell       *lc;
    bool            eqQualHere;

    foreach(lc, path->indexclauses)
    {
        IndexClause    *iclause = lfirst_node(IndexClause, lc);
        ListCell       *lc2;

        foreach(lc2, iclause->indexquals)
        {
            RestrictInfo   *rinfo = lfirst_node(RestrictInfo, lc2);
            Expr           *clause = rinfo->clause;
            Oid             clause_op = InvalidOid;

            // TODO maybe, it must be OpExpr.
            if (IsA(clause, OpExpr))
            {
                OpExpr *op = (OpExpr *) clause;
                clause_op = op->opno;
            }

            // TODO if it is equal strategy
            eqQualHere = true;

            indexBoundQuals = lappend(indexBoundQuals, rinfo);
        }
    }

    if (index->unique && indexcol == index->nkeycolumns - 1 && eqQualHere)
    {
        numIndexTuples = 1.0;
    }
    else
    {
        List   *selectivityQuals;
        Selectivity btreeSelectivity;

        selectivityQuals = indexBoundQuals;

        // TODO selectivity

        numIndexTuples = btreeSelectivity * index->rel->tuples;
    }
    costs.numIndexTuples = numIndexTuples;

    genericcostestimate(root, path, loop_count, &costs);

    if (index->tuples > 1)
    {
        desccentCost = ceil(log(index->tuples) / log(2.0)) * cpu_operator_cost;
        costs.indexStartupCost += desccentCost;
        costs.indexTotalCost += desccentCost;
    }
    desccentCost = (index->tree_height + 1) * 50.0 * cpu_operator_cost;
    costs.indexStartupCost += desccentCost;
    costs.indexTotalCost += desccentCost;

    *indexStartupCost = costs.indexStartupCost;
    *indexTotalCost = costs.indexTotalCost;
    *indexSelectivity = costs.indexSelectivity;
    *indexPages = costs.numIndexPages;
}

void
genericcostestimate(PlannerInfo *root, IndexPath *path,
                    double loop_count, GenericCosts *costs)
{
    IndexOptInfo   *index = path->indexinfo;
    List           *indexQuals = get_quals_from_indexclauses(path->indexclauses);
    Cost            indexStartupCost;
    Cost            indexTotalCost;
    Selectivity     indexSelectivity;
    double          numIndexPages;
    double          numIndexTuples;
    double          spc_random_page_cost;
    double          num_outer_scans;
    double          num_scans;
    double          qual_op_cost;
    List           *selectivityQuals;

    selectivityQuals = indexQuals;

    // TODO indexSelectivity

    numIndexTuples = costs->numIndexTuples;
    if (numIndexTuples <= 0.0)
    {
        numIndexTuples = indexSelectivity * index->rel->tuples;
    }

    if (numIndexTuples > index->tuples)
    {
        numIndexTuples = index->tuples;
    }
    if (numIndexTuples < 1.0)
    {
        numIndexTuples = 1.0;
    }

    if (index->pages > 1 && index->tuples > 1)
    {
        numIndexPages = ceil(numIndexTuples * index->pages / index->tuples);
    }
    else
    {
        numIndexPages = 1.0;
    }

    get_page_costs(&spc_random_page_cost, NULL);
    num_outer_scans = loop_count;
    num_scans = num_outer_scans;

    if (num_scans > 1)
    {
        double      pages_fetched;
        pages_fetched = numIndexPages * num_scans;
        pages_fetched = index_pages_fetched(pages_fetched,
                                            index->pages,
                                            (double) index->pages,
                                            root);
        indexTotalCost = (pages_fetched * spc_random_page_cost)
                / num_outer_scans;
    }
    else
    {
        indexTotalCost = numIndexTuples * spc_random_page_cost;
    }

    qual_op_cost = cpu_operator_cost * list_length(indexQuals);

    indexStartupCost = 0;
    indexTotalCost += numIndexTuples * (cpu_index_tuple_cost + qual_op_cost);

    costs->indexStartupCost = indexStartupCost;
    costs->indexTotalCost = indexTotalCost;
    costs->indexSelectivity = indexSelectivity;
    costs->numIndexPages = numIndexPages;
    costs->numIndexTuples = numIndexTuples;
    costs->spc_random_page_cost = spc_random_page_cost;
}

List *
get_quals_from_indexclauses(List *indexclauses)
{
    List       *result = NIL;
    ListCell   *lc;

    foreach(lc, indexclauses)
    {
        IndexClause    *iclause = lfirst_node(IndexClause, lc);
        ListCell       *lc2;

        foreach(lc2, iclause->indexquals)
        {
            RestrictInfo *rinfo = lfirst_node(RestrictInfo, lc2);
            result = lappend(result, rinfo);
        }
    }
    return result;
}