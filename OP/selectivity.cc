//
// Created by zoey on 23-5-17.
//
#include <algorithm>
#include <set>
#include "ql.h"
#include "ql_iterator.h"

//计算单表的选择度
//输入: 表的序号relNum,简单条件列表 simpleConditions
double calSimpleSelectivity(int relNum,  const std::vector<std::vector<QL_Condition>>& simpleConditions) {
    int numConditions = simpleConditions[relNum].size();

    double selectivity = 1.0;

    if (numConditions == 1) {

        // 创建属性不同值个数变量diff_value，初始化为10
        int diff_values = 10;

        // 根据条件操作符计算选择率
        if (simpleConditions[relNum][0].op == EQ_OP) {
            selectivity = 1.0 / diff_values;
        }
        if (simpleConditions[relNum][0].op == LT_OP || simpleConditions[relNum][0].op == GT_OP ||
            simpleConditions[relNum][0].op == LE_OP || simpleConditions[relNum][0].op == GE_OP) {
            selectivity = 1.0 / 3.0;
        }
        if (simpleConditions[relNum][0].op == NE_OP) {
            selectivity = 1.0 - (1.0 / diff_values);
        }
    }
    if (numConditions > 1) {
        for (const auto& cond : simpleConditions[relNum]) {
            // 创建属性不同值个数变量diff_value，初始化为10
            int diff_values = 10;

            // 根据条件操作符计算选择率
            if (cond.op == EQ_OP) {
                selectivity *= (1.0 / diff_values);
            }
            if (cond.op == LT_OP || cond.op == GT_OP || cond.op == LE_OP || cond.op == GE_OP) {
                selectivity *= (1.0 / 3.0);
            }
            if (cond.op == NE_OP) {
                selectivity *= (1.0 - (1.0 / diff_values));
            }
        }
    }
    /*
    if (numConditions >= 1) {
    std::vector<double> selectivities;

    for (const auto& cond : simpleConditions[relNum]) {
        // 根据条件操作符计算选择率
        double conditionSelectivity = 1.0f;
        if (cond.op == EQ_OP) {
            // 获取属性不同值个数
            int diff_values = 10;  // 这里需要根据实际情况获取属性不同值个数
            conditionSelectivity = 1.0f / diff_values;
        } else if (cond.op == LT_OP || cond.op == GT_OP || cond.op == LE_OP || cond.op == GE_OP) {
            conditionSelectivity = 1.0f / 3.0f;
        } else if (cond.op == NE_OP) {
            int diff_values = 10;  // 这里需要根据实际情况获取属性不同值个数
            conditionSelectivity = 1.0f - (1.0f / diff_values);
        }

        selectivities.push_back(conditionSelectivity);
    }

    // 对选择率从大到小排序
    std::sort(selectivities.rbegin(), selectivities.rend());

    // 计算总选择率
    for (size_t i = 0; i < selectivities.size(); ++i) {
        selectivity *= std::pow(selectivities[i], 1.0f / std::pow(2, i));
    }
}*/
    return selectivity;
}

//计算连接的选择度
//输入: 左关系的选择度, 右关系的选择度
double calJoinSelectivity(double leftSelectivity, double rightSelectivity){
    // 取最小值作为连接后关系的选择率
    double joinSelectivity = std::min(leftSelectivity, rightSelectivity);

    // 返回连接后关系的选择率
    return joinSelectivity;
}