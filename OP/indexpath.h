//
// Created by elias on 23-5-3.
//

#ifndef MICRODBMS_INDEXPATH_H
#define MICRODBMS_INDEXPATH_H

#include "../utils/list.h"

typedef struct
{
    bool    nonempty;
    List   *indexclauses;
} IndexClauseSet;

#endif //MICRODBMS_INDEXPATH_H
