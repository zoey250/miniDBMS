//
// Created by elias on 23-5-4.
//

#include <math.h>
#include "index_selfuncs.h"
#include "selfuncs.h"
#include "optimizer.h"
#include "selectivity.h"

#define BTLessStrategyNumber			1
#define BTLessEqualStrategyNumber		2
#define BTEqualStrategyNumber			3
#define BTGreaterEqualStrategyNumber	4
#define BTGreaterStrategyNumber			5

static int get_op_strategy(CompOp opno);
static Selectivity btselectivityestimate();

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
    bool            eqQualHere;

    for (int i = 0; i < path->indexinfo->conditions.size(); ++i)
    {
        int             op_strategy;

        op_strategy = get_op_strategy(path->indexinfo->conditions[i].op);
        if (op_strategy == BTEqualStrategyNumber)
        {
            eqQualHere = true;
        }
    }

    if (index->unique && eqQualHere)
    {
        index->flag = true;
        numIndexTuples = 1.0;
        costs.indexSelectivity = 1.0 / path->indexinfo->tuples;
    }
    else
    {
        Selectivity btreeSelectivity = 1.0;

        btreeSelectivity = calSingleRelSelectivity(path->indexinfo->conditions);

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
    path->path.rows = numIndexTuples;

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
    Cost            indexStartupCost;
    Cost            indexTotalCost;
    Selectivity     indexSelectivity = 1.0;
    double          numIndexPages;
    double          numIndexTuples;
    double          spc_random_page_cost;
    double          num_outer_scans;
    double          num_scans;
    double          qual_op_cost;

    indexSelectivity = calSingleRelSelectivity(path->indexinfo->conditions);

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
        pages_fetched = index_pages_fetched(path, index->pages, (double) index->pages);
        indexTotalCost = (pages_fetched * spc_random_page_cost)
                / num_outer_scans;
    }
    else
    {
        indexTotalCost = numIndexTuples * spc_random_page_cost;
    }

    qual_op_cost = cpu_operator_cost * path->indexinfo->conditions.size();

    indexStartupCost = 0;
    indexTotalCost += numIndexTuples * (cpu_index_tuple_cost + qual_op_cost);

    costs->indexStartupCost = indexStartupCost;
    costs->indexTotalCost = indexTotalCost;
    costs->numIndexPages = 1.0;
    if (costs->indexSelectivity == 0.0)
    {
        costs->indexSelectivity = indexSelectivity;
        costs->numIndexPages = numIndexPages;
    }
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

static int
get_op_strategy(CompOp opno)
{
    int result;
    switch (opno)
    {
        case EQ_OP:
            result = BTEqualStrategyNumber;
            break;
        case LT_OP:
            result = BTLessEqualStrategyNumber;
            break;
        case GT_OP:
            result = BTGreaterStrategyNumber;
            break;
        case LE_OP:
            result = BTLessEqualStrategyNumber;
            break;
        case GE_OP:
            result = BTGreaterEqualStrategyNumber;
            break;
        default:
            result = 0;
            break;
    }
    return result;
}

static Selectivity
btselectivityestimate(IndexPath *path)
{

}
