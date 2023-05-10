//
// Created by elias on 23-5-5.
//

#ifndef MICRODBMS_SELFUNCS_H
#define MICRODBMS_SELFUNCS_H

#include "cost.h"

typedef struct
{
    Cost        indexStartupCost;
    Cost        indexTotalCost;
    Selectivity indexSelectivity;

    double      numIndexPages;
    double      numIndexTuples;
    double      spc_random_page_cost;
} GenericCosts;

extern void genericcostestimate(PlannerInfo *root, IndexPath *path,
                                double loop_count, GenericCosts *costs);
extern List *get_quals_from_indexclauses(List *indexclauses);

#endif //MICRODBMS_SELFUNCS_H
