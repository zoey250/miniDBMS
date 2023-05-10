//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_COST_H
#define MICRODBMS_COST_H

#include "pathnodes.h"

#define DEFAULT_SEQ_PAGE_COST 1.0
#define DEFAULT_RANDOM_PAGE_COST 4.0

#define DEFAULT_CPU_TUPLE_COST 0.01
#define DEFAULT_CPU_INDEX_TUPLE_COST 0.005
#define DEFAULT_CPU_OPERATOR_COST 0.0025
#define DEFAULT_EFFECTIVE_CACHE_SIZE  524288

extern double index_pages_fetched(double tuples_fetched, BlockNumber pages,
                                  double index_pages, PlannerInfo *root);
extern void cost_seqscan(Path *path, RelOptInfo *baserel);
extern void cost_index(IndexPath *path, PlannerInfo *root,
                       double loop_count);
extern void get_page_costs(double *spc_random_page_cost,
                           double *spc_seq_page_cost);

#endif //MICRODBMS_COST_H
