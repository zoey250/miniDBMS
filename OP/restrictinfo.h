//
// Created by elias on 23-5-8.
//

#ifndef MICRODBMS_RESTRICTINFO_H
#define MICRODBMS_RESTRICTINFO_H

#include "pathnodes.h"
#include "../redbase.h"

extern RestrictInfo *commute_restrictinfo(RestrictInfo *rinfo,
                                          CompOp comm_op);
extern CompOp get_commutator(CompOp opno);

#endif //MICRODBMS_RESTRICTINFO_H
