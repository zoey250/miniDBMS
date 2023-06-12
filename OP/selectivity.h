//
// Created by zoey on 23-5-17.
//

#ifndef SELECTIVITY_H
#define SELECTIVITY_H

#include <algorithm>
#include <set>
#include "ql.h"
#include "ql_iterator.h"
#include <complex>
#include "../statistics/statistics.h"

int findIntegerPosition(const std::string& valueString, int targetInteger, int& start, int& end);
void findMinMax(const std::string& valueString, int& min, int& max);
std::string replaceCommaWithDash(const std::string& input);
extern double calSingleRelSelectivity(const std::vector<QL_Condition>& conditions);
extern double calJoinSelectivity(double leftSelectivity, double rightSelectivity);

#endif //SELECTIVITY_H

