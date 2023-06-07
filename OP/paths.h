//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_PATHS_H
#define MICRODBMS_PATHS_H

#include <set>
#include "pathnodes.h"
#include "rm.h"
#include "sm_catalog.h"
#include "ql_internal.h"
#include "ix.h"

extern PlannerInfo *init_planner_info(AttrList finalProjections,
                                      std::vector<AttrList> simpleProjections,
                                      int nRelations, const char *const *relations,
                                      std::vector<RM_FileHandle> fileHandles,
                                      std::vector<RelCatEntry> relEntries,
                                      std::vector<std::vector<std::pair<IX_IndexHandle, IndexOptInfo>>> indexVector,
                                      std::vector<std::vector<QL_Condition>> simpleConditions,
                                      std::vector<QL_Condition> complexConditions,
                                      std::vector<AttrList> attrInfo);

extern void cost_estimate(PlannerInfo *root);

extern bool match_index_to_operand(Node *operand, IndexOptInfo *index);

#endif //MICRODBMS_PATHS_H
