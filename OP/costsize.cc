//
// Created by elias on 23-5-1.
//

#include <math.h>
#include "cost.h"
#include "index_selfuncs.h"
#include "optimizer.h"

double seq_page_cost = DEFAULT_SEQ_PAGE_COST;
double random_page_cost = DEFAULT_RANDOM_PAGE_COST;

double cpu_tuple_cost = DEFAULT_CPU_TUPLE_COST;
double cpu_index_tuple_cost = DEFAULT_CPU_INDEX_TUPLE_COST;
double cpu_operator_cost = DEFAULT_CPU_OPERATOR_COST;

int effective_cache_size = DEFAULT_EFFECTIVE_CACHE_SIZE;

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
        pages_fetched = index_pages_fetched(tuples_fetched * loop_count,
                                            baserel->pages,
                                            (double) index->pages,
                                            root);
        rand_heap_pages = pages_fetched;

        max_IO_cost = (pages_fetched * spc_random_page_cost) / loop_count;

        pages_fetched = ceil(indexSelectivity * (double) baserel->pages);
        pages_fetched = index_pages_fetched(pages_fetched * loop_count,
                                            baserel->pages,
                                            (double) index->pages,
                                            root);
        min_IO_cost = (pages_fetched * spc_random_page_cost) / loop_count;
    }
    else
    {
        pages_fetched = index_pages_fetched(tuples_fetched,
                                            baserel->pages,
                                            (double) index->pages,
                                            root);

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

    // TODO csquared 不打算做了
    run_cost += max_IO_cost;

    startup_cost += qpqual_cost.startup;
    cpu_per_tuple = cpu_tuple_cost + qpqual_cost.per_tuple;
    cpu_run_cost += cpu_per_tuple * tuples_fetched;

    /*
     * TODO
    startup_cost += path->path.pathtarget->cost.startup;
    cpu_run_cost += path->path.pathtarget->cost. per_tuple * path->path.rows;
     */

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
index_pages_fetched(double tuples_fetched, BlockNumber pages,
                    double index_pages, PlannerInfo *root)
{
    double  pages_fetched;
    double  total_pages;
    double  T;
    double  b;

    T = (pages > 1) ? (double) pages : 1.0;

    total_pages = root->total_table_pages + index_pages;
    total_pages = Max(total_pages, 1.0);

    b = (double) effective_cache_size * T / total_pages;

    if (b <= 1.0)
    {
        b = 1.0;
    }
    else
    {
        b = ceil(b);
    }

    if (T <= b)
    {
        pages_fetched =
                (2.0 * T * tuples_fetched) / (2.0 * T + tuples_fetched);
        if (pages_fetched >= T)
        {
            pages_fetched = T;
        }
        else
        {
            pages_fetched = ceil(pages_fetched);
        }
    }
    else
    {
        double  lim;

        lim = (2.0 * T * b) / (2.0 * T - b);
        if (tuples_fetched <= lim)
        {
            pages_fetched = (2.0 * T * tuples_fetched) / (2.0 * T + tuples_fetched);
        }
        else
        {
            pages_fetched = b + (tuples_fetched - lim) * (T - b) / T;
        }
        pages_fetched = ceil(pages_fetched);
    }
    return pages_fetched;
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

    // TODO selectivity
    nrows = selectivity * rel->tuples;

    rel->rows = clamp_row_est(nrows);
}