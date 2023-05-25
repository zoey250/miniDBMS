//
// Created by elias on 23-5-1.
//

#include <math.h>
#include "cost.h"
#include "index_selfuncs.h"
#include "optimizer.h"
#include "selectivity.h"

double seq_page_cost = DEFAULT_SEQ_PAGE_COST;
double random_page_cost = DEFAULT_RANDOM_PAGE_COST;

double cpu_tuple_cost = DEFAULT_CPU_TUPLE_COST;
double cpu_index_tuple_cost = DEFAULT_CPU_INDEX_TUPLE_COST;
double cpu_operator_cost = DEFAULT_CPU_OPERATOR_COST;

int effective_cache_size = DEFAULT_EFFECTIVE_CACHE_SIZE;

static void cost_rescan(Path *path, Cost *rescan_startup_cost, Cost *rescan_total_cost);
static Cardinality cal_nrows(NestPath *path, Cardinality ntuples);
static inline bool is_leaf(Path *path);

void
cost_seqscan(Path *path, RelOptInfo *baserel)
{
    Cost    startup_cost = 0;
    Cost    cpu_run_cost;
    Cost    disk_run_cost;

    double  spc_seq_page_cost;
    QualCost    qpqual_cost;
    Cost    cpu_per_tuple;

    path->rows = baserel->rows;

    get_page_costs(NULL, &spc_seq_page_cost);

    disk_run_cost = spc_seq_page_cost * baserel->pages;

    qpqual_cost = baserel->baserestrictcost;

    startup_cost += qpqual_cost.startup;
    cpu_per_tuple = cpu_tuple_cost + qpqual_cost.per_tuple;
    cpu_run_cost = cpu_per_tuple * baserel->tuples;

    path->startup_cost = startup_cost;
    path->total_cost = startup_cost + cpu_run_cost + disk_run_cost;
}

void
cost_index(IndexPath *path, PlannerInfo *root, double loop_count)
{
    IndexOptInfo   *index = path->indexinfo;
    RelOptInfo     *baserel = index->rel;
    Cost            startup_cost = 0;
    Cost            run_cost = 0;
    Cost            cpu_run_cost = 0;

    Cost            indexStartupCost;
    Cost            indexTotalCost;
    Selectivity     indexSelectivity;

    double          spc_seq_page_cost;
    double          spc_random_page_cost;

    Cost            min_IO_cost;
    Cost            max_IO_cost;

    QualCost        qpqual_cost;
    Cost            cpu_per_tuple;

    double          tuples_fetched;
    double          pages_fetched;

    double          rand_heap_pages;
    double          index_pages;

    path->path.rows = baserel->rows;

    btcostestimate(root, path, loop_count, &indexStartupCost,
                   &indexTotalCost, &indexSelectivity,
                   &index_pages);

    path->indextotalcost = indexTotalCost;
    path->indexselectivity = indexSelectivity;

    startup_cost += indexStartupCost;
    run_cost += indexTotalCost - indexStartupCost;

    tuples_fetched = clamp_row_est(indexSelectivity * baserel->tuples);

    get_page_costs(&spc_random_page_cost, &spc_seq_page_cost);

    if (loop_count > 1)
    {
        pages_fetched = index_pages_fetched(path, baserel->pages, (double) index->pages);
        rand_heap_pages = pages_fetched;

        max_IO_cost = (pages_fetched * spc_random_page_cost) / loop_count;

        pages_fetched = ceil(indexSelectivity * (double) baserel->pages);
        pages_fetched = index_pages_fetched(path, baserel->pages, (double) index->pages);
        min_IO_cost = (pages_fetched * spc_random_page_cost) / loop_count;
    }
    else
    {
        pages_fetched = index_pages_fetched(path, baserel->pages, (double) index->pages);

        rand_heap_pages = pages_fetched;
        max_IO_cost = pages_fetched * spc_random_page_cost;
        pages_fetched = ceil(indexSelectivity * (double) baserel->pages);
        if (pages_fetched > 0)
        {
            min_IO_cost = spc_random_page_cost;
            if (pages_fetched > 1)
            {
                min_IO_cost += (pages_fetched - 1) * spc_seq_page_cost;
            }
        }
        else
        {
            min_IO_cost = 0;
        }
    }

    run_cost += max_IO_cost;

    startup_cost += qpqual_cost.startup;
    cpu_per_tuple = cpu_tuple_cost + qpqual_cost.per_tuple;
    cpu_run_cost += cpu_per_tuple * tuples_fetched;

    run_cost += cpu_run_cost;

    path->path.startup_cost = startup_cost;
    path->path.total_cost = startup_cost + run_cost;
}

void
get_page_costs(double *spc_random_page_cost, double *spc_seq_page_cost)
{
    if (spc_random_page_cost)
    {
        *spc_random_page_cost = random_page_cost;
    }

    if (spc_seq_page_cost)
    {
        *spc_seq_page_cost = seq_page_cost;
    }
}

double
index_pages_fetched(IndexPath *path, BlockNumber pages, double index_pages)
{
    if (path->indexinfo->flag)
    {
        return 2;
    }
    return index_pages * path->indexselectivity + pages;
}

double
clamp_row_est(double nrows)
{
    if (nrows <= 1.0)
    {
        nrows = 1.0;
    }
    else
    {
        nrows = rint(nrows);
    }
    return nrows;
}

void
set_baserel_size_estimates(RelOptInfo *rel)
{
    double  nrows = 0.0;
    double  selectivity = 1.0;

    selectivity = calSingleRelSelectivity(rel->conditions);
    nrows = selectivity * rel->tuples;

    rel->rows = clamp_row_est(nrows);
}

void
cost_nestloop(NestPath *path)
{
    Path   *outer_path = path->jpath.outerjoinpath;
    Path   *inner_path = path->jpath.innerjoinpath;
    Cost    inner_rescan_start_cost;
    Cost    inner_rescan_total_cost;
    Cost    inner_run_cost;
    Cost    inner_rescan_run_cost;

    Cost    cpu_per_tuple;

    Cost    startup_cost = 0;
    Cost    run_cost = 0;

    Cardinality outer_path_rows = outer_path->rows;
    Cardinality inner_path_rows = inner_path->rows;
    Cardinality ntuples;
    Cardinality nrows;

    if (outer_path_rows <= 0)
    {
        outer_path_rows = 1;
    }
    if (inner_path_rows <= 0)
    {
        inner_path_rows = 1;
    }

    cost_rescan(inner_path,
                &inner_rescan_start_cost,
                &inner_rescan_total_cost);

    startup_cost += outer_path->startup_cost + inner_path->startup_cost;
    run_cost += outer_path->total_cost - outer_path->startup_cost;

    if (outer_path_rows > 1)
    {
        run_cost +=  inner_rescan_start_cost * (outer_path_rows - 1);
    }

    inner_run_cost = inner_path->total_cost - inner_path->startup_cost;
    inner_rescan_run_cost = inner_rescan_total_cost - inner_rescan_start_cost;

    run_cost += inner_run_cost;
    if (outer_path_rows > 1)
    {
        run_cost += inner_rescan_run_cost * (outer_path_rows - 1);
    }
    ntuples = outer_path_rows * inner_path_rows;

    cpu_per_tuple = cpu_tuple_cost;
    run_cost += cpu_per_tuple * ntuples;

    nrows = cal_nrows(path, ntuples);

    path->jpath.path.startup_cost = startup_cost;
    path->jpath.path.total_cost = startup_cost + run_cost;
    path->jpath.path.rows = nrows;
}

static void
cost_rescan(Path *path, Cost *rescan_startup_cost, Cost *rescan_total_cost)
{
    *rescan_startup_cost = path->startup_cost;
    *rescan_total_cost = path->total_cost;
}

static Cardinality
cal_nrows(NestPath *path, Cardinality ntuples)
{
    Path   *inner_path = path->jpath.outerjoinpath;
    Path   *outer_path = path->jpath.innerjoinpath;
    Selectivity inner_selectivity = 1.0;
    Selectivity outer_selectivity = 1.0;
    Selectivity selectivity = 1.0;

    if (is_leaf(inner_path))
    {
        inner_selectivity = calSingleRelSelectivity(inner_path->parent->complexconditions);
    }

    if (is_leaf(outer_path))
    {
        outer_selectivity = calSingleRelSelectivity(outer_path->parent->complexconditions);
    }

    selectivity = calJoinSelectivity(outer_selectivity, inner_selectivity);

    return ntuples * selectivity;
}

static inline bool
is_leaf(Path *path)
{
    if (path->pathtype == T_SeqScan ||
        path->pathtype == T_IndexScan)
    {
        return true;
    }
    return false;
}