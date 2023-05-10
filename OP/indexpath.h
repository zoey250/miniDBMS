//
// Created by elias on 23-5-3.
//

#ifndef MICRODBMS_INDEXPATH_H
#define MICRODBMS_INDEXPATH_H

#include "list.h"

// TODO maybe change
#define INDEX_MAX_KEYS 32

typedef struct
{
    bool    nonempty;
    List   *indexclauses[INDEX_MAX_KEYS];
} IndexClauseSet;

#endif //MICRODBMS_INDEXPATH_H
