//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_PATHS_H
#define MICRODBMS_PATHS_H

#include "pathnodes.h"
#include "rm.h"
#include "sm_catalog.h"
#include "ql_internal.h"

extern PlannerInfo *init_planner_info(int nSelAttrs, const RelAttr *selAttrs,
                  int nRelations, const char *const *relations,
                  const std::vector<RM_FileHandle> fileHandles,
                  std::vector<RelCatEntry> relEntries,
                  std::vector<AttrList> attrInfo,
                  int nConditions, const Condition *conditions);

extern void cost_estimate(PlannerInfo *root);

extern bool match_index_to_operand(Node *operand, IndexOptInfo *index);

#endif //MICRODBMS_PATHS_H
