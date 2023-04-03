//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H

#include <stdlib.h>
#include <string.h>
#include <ostream>
#include "../redbase.h"
#include "../PARSER/parser.h"
#include "../RM/rm.h"
#include "../IX/ix.h"
#include "../SM/sm.h"
#include "ql_internal.h"

//
// QL_Manager: query language (DML)
//
class QL_Manager {
public:
    // 构造函数
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    // 析构函数
    ~QL_Manager();

    // 选择操作
    RC Select  (int nSelAttrs,                   // select属性的个数
                const RelAttr selAttrs[],        // 属性列表
                int   nRelations,                // from关系表个数
                const char * const relations[],  // 关系表列表
                int   nConditions,               // where条件个数
                const Condition conditions[]);   // 条件列表

    // 插入操作
    RC Insert  (const char *relName,             // 要插入的关系表名
                int   nValues,                   // 要插入的值的个数
                const Value values[]);           // 值数组

    // 删除操作
    RC Delete  (const char *relName,             // 要删除数据的关系表名
                int   nConditions,               // where条件个数
                const Condition conditions[]);   // 条件列表

    // 更新操作
    RC Update  (const char *relName,             // 更新的关系表名
                const RelAttr &updAttr,          // 要更新的属性
                const int bIsValue,              // 是否是常量标识符 =1表示右侧为常量 =0表示右侧为属性
                const RelAttr &rhsRelAttr,       // 右侧属性
                const Value &rhsValue,           // 右侧常量
                int   nConditions,               // where条件个数
                const Condition conditions[]);   // 条件列表

private:
    SM_Manager *pSmm;   // SM_Manager对象
    IX_Manager *pIxm;   // IX_Manager对象
    RM_Manager *pRmm;   // RM_Manager对象

    // 检查条件是否合法
    RC CheckConditionsValid(const char *relName, int nConditions, const Condition *conditions,
                            const std::map<std::string, DataAttrInfo> &attrMap,
                            std::vector<QL_Condition> &retConditions);
};

// 输出错误信息函数
void QL_PrintError(RC rc);

bool checkSatisfy(char *lhsData, bool lhsIsnull, char *rhsData, bool rhsIsnull, const QL_Condition &condition);
bool checkSatisfy(char *data, bool *isnull, const QL_Condition &condition);

#define QL_ATTR_COUNT_MISMATCH      (START_QL_WARN + 0)
#define QL_VALUE_TYPES_MISMATCH     (START_QL_WARN + 1)
#define QL_STRING_VAL_TOO_LONG      (START_QL_WARN + 2)
#define QL_ATTR_NOTEXIST            (START_QL_WARN + 3)
#define QL_ATTR_TYPES_MISMATCH      (START_QL_WARN + 4)
#define QL_AMBIGUOUS_ATTR_NAME      (START_QL_WARN + 5)
#define QL_FORBIDDEN                (START_QL_WARN + 6)
#define QL_ATTR_IS_NOTNULL          (START_QL_WARN + 7)
#define QL_DUPLICATE_PRIMARY_KEY    (START_QL_WARN + 8)
#define QL_LASTWARN QL_DUPLICATE_PRIMARY_KEY

#define QL_SOMEERROR                (START_QL_ERR - 0)
#define QL_LASTERROR QL_SOMEERROR

#endif // QL_H
