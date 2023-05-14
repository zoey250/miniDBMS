//
// Created by elias on 23-5-5.
//

#ifndef MICRODBMS_OPTIMIZER_H
#define MICRODBMS_OPTIMIZER_H

extern double cpu_operator_cost;
extern double cpu_index_tuple_cost;

extern double clamp_row_est(double nrows);

#endif //MICRODBMS_OPTIMIZER_H
