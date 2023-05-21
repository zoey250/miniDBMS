//
// Created by zoey on 23-5-17.
//

#ifndef SELECTIVITY_H
#define SELECTIVITY_H

#include <algorithm>
#include <set>
#include "ql.h"
#include "ql_iterator.h"

extern double calSingleRelSelectivity(const std::vector<QL_Condition>& conditions);
extern double calJoinSelectivity(double leftSelectivity, double rightSelectivity);

#endif //SELECTIVITY_H

