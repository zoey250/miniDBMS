//
// Created by zoey on 23-5-17.
//

#ifndef SELECTIVITY_H
#define SELECTIVITY_H

#include <algorithm>
#include <set>
#include "ql.h"
#include "ql_iterator.h"

double calSimpleSelectivity(int relNum,  const std::vector<std::vector<QL_Condition>>& simpleConditions);
double calJoinSelectivity(double leftSelectivity, double rightSelectivity);

#endif //SELECTIVITY_H

