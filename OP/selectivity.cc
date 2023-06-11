//
// Created by zoey on 23-5-17.
//
#include "selectivity.h"

// 将value中括号内的逗号替换为连字符,方便后续处理
std::string replaceCommaWithDash(const std::string& input) {
    std::string result = input;
    bool withinParentheses = false;

    for (char& c : result) {
        if (c == '(')
            withinParentheses = true;
        else if (c == ')')
            withinParentheses = false;

        if (withinParentheses && c == ',')
            c = '-';
    }

    return result;
}

// 查找特定值在直方图中对应的桶号
int findIntegerPosition(const std::string& valueString, int targetInteger, int& start, int& end) {

    // 将括号内的逗号替换为连字符
    std::string valueString1 = replaceCommaWithDash(valueString);

    // 移除括号和空格，并将字符串拆分成括号内的数值对
    std::string cleanValueString;
    for (char c : valueString1) {
        if (c != '(' && c != ')' && c != ' ') {
            cleanValueString += c;
        }
    }

    // 拆分数值对并存储到 vector 中
    std::vector<std::string> valuePairs;
    std::string currentValuePair;
    for (char c : cleanValueString) {
        if (c == ',') {
            valuePairs.push_back(currentValuePair);
            currentValuePair.clear();
        } else {
            currentValuePair += c;
        }
    }
    valuePairs.push_back(currentValuePair);

    // 遍历拆分后的数值对列表，查找目标整数值
    for (int i = 0; i < valuePairs.size(); ++i) {
        std::string valuePair = valuePairs[i];
        size_t dashPos = valuePair.find('-');
        start = std::stoi(valuePair.substr(0, dashPos));
        end = std::stoi(valuePair.substr(dashPos + 1));
        if (start <= targetInteger && targetInteger <= end) {
            // 找到目标整数值所在的括号位置，返回索引位置（从0开始）
            return i+1;
        }
    }

    // 如果目标整数值未在字符串中找到，返回-1表示未找到
    return -1;
}

// 查找桶的最大值和最小值
void findMinMax(const std::string& valueString, int& min, int& max) {
    std::vector<int> values;

    // 用逗号分割字符串，获取每对括号内的数字
    std::stringstream ss(valueString);
    std::string token;
    while (getline(ss, token, ',')) {
        // 去除括号和空格，并将字符串转换为整数
        token.erase(std::remove(token.begin(), token.end(), '('), token.end());
        token.erase(std::remove(token.begin(), token.end(), ')'), token.end());
        token.erase(std::remove(token.begin(), token.end(), ' '), token.end());
        int value = std::stoi(token);
        values.push_back(value);
    }

    // 找到最左面的值和最右面的值
    min = *std::min_element(values.begin(), values.end());
    max = *std::max_element(values.begin(), values.end());
}

double calSingleRelSelectivity(const std::vector<QL_Condition>& conditions) {
    int numConditions = conditions.size();
    double conditionSelectivity = 1.0;

    if (numConditions >= 1) {
        // 也适用只有一个condition的情况
        for (const auto& cond : conditions) {

            int rhsValue = *static_cast<int*>(cond.rhsValue.data);

            // int类型用直方图
            if(cond.lhsAttr.attrType == INT && !statisticsData.empty()){

                int bucketNum = 0;
                // bucketPositon从1开始
                int bucketPosition = 0;
                int bucketLeftValue;
                int bucketRightValue;
                int maxValue, minValue;

                for (const auto& data : statisticsData) {

                    if (data.relName == cond.lhsAttr.relName && data.attrName == cond.lhsAttr.attrName) {
                        // 找到匹配的relName和attrName
                        bucketNum = data.bucketNum;
                        bucketPosition = findIntegerPosition(data.value, rhsValue, bucketLeftValue, bucketRightValue);
                        std::cout<<"rhsValue: "<<rhsValue<<std::endl;
                        std::cout<<"data.value: "<<data.value<<std::endl;
                        std::cout<<"bucketPosition: "<< bucketPosition<<std::endl;
                        std::cout<<"bucketLeftValue: "<<bucketLeftValue<<std::endl;
                        std::cout<<"bucketRightValue: "<<bucketRightValue<<std::endl;
                        findMinMax(data.value, minValue, maxValue);
                        if(bucketNum == 0){
                            printf("直方图桶信息获取错误");
                            return -1;
                        }
                        break;
                    }
                }

                if (cond.op == EQ_OP){

                    if(bucketPosition == -1){
                        // 选择度为0
                        return 0;
                    }

                    if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= (1.0/bucketNum);
                    }
                    else{
                        conditionSelectivity *= (1.0/(bucketNum*(bucketRightValue-bucketLeftValue)));
                    }
                }

                if (cond.op == NE_OP){

                    if(bucketPosition == -1){
                        // 选择度为1
                        conditionSelectivity *= 1.0;
                    }

                    else if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= (1.0 - 1.0/bucketNum);
                    }else{
                        conditionSelectivity *= (1.0 - 1.0/(bucketNum*(bucketRightValue-bucketLeftValue)));
                    }
                }

                if(cond.op == LT_OP){

                    if(rhsValue <= minValue){
                        return 0;
                    }

                    if(rhsValue >= maxValue){
                        conditionSelectivity *= 1;
                    }

                    else if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= ((bucketPosition-1.0) * (1.0/bucketNum));
                    }else{
                        conditionSelectivity *= ((bucketPosition-1.0 + ((rhsValue - bucketLeftValue)*1.0/(bucketRightValue - bucketLeftValue))) * (1.0/bucketNum));
                    }
                }

                if(cond.op == LE_OP){

                    if(rhsValue < minValue){
                        return 0;
                    }

                    if(rhsValue >= maxValue){
                        conditionSelectivity *= 1;
                    }

                    else if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= (bucketPosition * (1.0/bucketNum));
                    }else{
                        if(rhsValue == minValue){
                            conditionSelectivity *= (1.0/(bucketNum*(bucketRightValue-bucketLeftValue)));
                        }else{
                            conditionSelectivity *= ((bucketPosition-1.0 + ((rhsValue - bucketLeftValue)*1.0/(bucketRightValue - bucketLeftValue))) * (1.0/bucketNum));
                        }
                    }
                }

                if(cond.op == GT_OP){

                    if(rhsValue >= maxValue){
                        return 0;
                    }

                    if(rhsValue <= minValue){
                        conditionSelectivity *= 1;
                    }

                    else if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= ((bucketNum-bucketPosition)*1.0/bucketNum);
                    }else{
                        conditionSelectivity *= ((bucketNum-bucketPosition + (bucketRightValue-rhsValue)*1.0/(bucketRightValue-bucketLeftValue))*1.0/bucketNum);
                    }
                }

                if(cond.op == GE_OP){

                    if(rhsValue > maxValue){
                        return 0;
                    }

                    if(rhsValue <= minValue){
                        conditionSelectivity *= 1;
                    }

                    else if(bucketLeftValue == bucketRightValue){
                        conditionSelectivity *= (1.0 - (bucketPosition-1) * (1.0/bucketNum));
                    }
                    else{
                        if(rhsValue == maxValue){
                            conditionSelectivity *= (1.0/(bucketNum*(bucketRightValue-bucketLeftValue)));
                        }else{
                            conditionSelectivity *= (1.0 - ((bucketPosition-1.0 + (rhsValue - bucketLeftValue)*1.0/(bucketRightValue - bucketLeftValue)) * (1.0/bucketNum)));
                        }

                    }
                }

            }
            else{
                if (cond.op == EQ_OP) {
                    // 获取属性不同值个数
                    int diff_values = 10;  // 这里需要根据实际情况获取属性不同值个数
                    conditionSelectivity *= 1.0 / diff_values;
                } else if (cond.op == LT_OP || cond.op == GT_OP || cond.op == LE_OP || cond.op == GE_OP) {
                    conditionSelectivity *= 1.0 / 3.0;
                } else if (cond.op == NE_OP) {
                    int diff_values = 10;  // 这里需要根据实际情况获取属性不同值个数
                    conditionSelectivity *= 1.0 - (1.0 / diff_values);
                }
            }
        }
    }

    std::cout << "Selectivity: " << conditionSelectivity << std::endl;
    return conditionSelectivity;
}


double calJoinSelectivity(double leftSelectivity, double rightSelectivity){
    // 取最小值作为连接后关系的选择率
    double joinSelectivity = std::min(leftSelectivity, rightSelectivity);

    // 返回连接后关系的选择率
    return joinSelectivity;
}