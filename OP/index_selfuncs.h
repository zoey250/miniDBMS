//
// Created by elias on 23-5-4.
//

#ifndef MICRODBMS_INDEX_SELFUNCS_H
#define MICRODBMS_INDEX_SELFUNCS_H

#include "cost.h"

extern void btcostestimate(struct PlannerInfo *root,
                           struct IndexPath *path,
                           double loop_count,
                           Cost *indexStartupCost,
                           Cost *indexTotalCost,
                           Selectivity *indexSelectivity,
                           double *indexPages);
extern void complex_btcostestimate(struct PlannerInfo *root, struct IndexPath *path,
                           double loop_count, Cost *indexStartupCost,
                           Cost *indexTotalCost, Selectivity *indexSelectivity,
                           double *indexPages, QL_Condition condition);

#endif //MICRODBMS_INDEX_SELFUNCS_H
