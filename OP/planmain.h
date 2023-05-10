//
// Created by elias on 23-5-7.
//

#ifndef MICRODBMS_PLANMAIN_H
#define MICRODBMS_PLANMAIN_H

#include "pathnodes.h"

extern RestrictInfo *build_implied_join_equality(PlannerInfo *root,
                                                 Oid opno,
                                                 Expr *item1,
                                                 Expr *item2,
                                                 Relids qualscope);

#endif //MICRODBMS_PLANMAIN_H
