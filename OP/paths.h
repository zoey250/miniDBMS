//
// Created by elias on 23-5-1.
//

#ifndef MICRODBMS_PATHS_H
#define MICRODBMS_PATHS_H

#include "pathnodes.h"

typedef enum
{
    PATHKEYS_EQUAL,
    PATHKEYS_BETTER1,
    PATHKEYS_BETTER2,
    PATHKEYS_DIFFERENT
} PathKeysComparison;

extern PathKeysComparison compare_pathkeys(List *keys1, List *keys2);
extern bool match_index_to_operand(Node *operand, int indexcol, IndexOptInfo *index);
extern bool is_redundant_with_indexclauses(RestrictInfo *rinfo, List *indexclauses);
extern List *generate_implied_equalities_for_column(PlannerInfo *root,
                                                     RelOptInfo *rel,
                                                     void *callback_arg);

#endif //MICRODBMS_PATHS_H
