//
// Created by elias on 23-5-8.
//

#ifndef MICRODBMS_RESTRICTINFO_H
#define MICRODBMS_RESTRICTINFO_H

#include "pathnodes.h"

extern RestrictInfo *make_restrictinfo(PlannerInfo *root, Expr *clause, Relids required_relids);

#endif //MICRODBMS_RESTRICTINFO_H
