#ifndef _HELPER
#define _HELPER

#include <iostream>
#include <cstring>
#include <vector>
#include "../redbase.h"

#define MAXPRINTSTRING  ((2*MAXNAME) + 5)

//存储属性目录中保存的信息
struct DataAttrInfo {
    //构造函数
    DataAttrInfo() {
        memset(relName, 0, MAXNAME + 1);
        memset(attrName, 0, MAXNAME + 1);
    };

    DataAttrInfo( const DataAttrInfo &d ) {
        strcpy (relName, d.relName);
        strcpy (attrName, d.attrName);
        offset = d.offset;
        attrType = d.attrType;
        attrSize = d.attrSize;
        attrDisplayLength = d.attrDisplayLength;
        attrSpecs = d.attrSpecs;
        indexNo = d.indexNo;
        nullableIndex = d.nullableIndex;
    };

    DataAttrInfo& operator=(const DataAttrInfo &d) {
        if (this != &d) {
            strcpy (relName, d.relName);
            strcpy (attrName, d.attrName);
            offset = d.offset;
            attrType = d.attrType;
            attrSize = d.attrSize;
            attrDisplayLength = d.attrDisplayLength;
            attrSpecs = d.attrSpecs;
            indexNo = d.indexNo;
            nullableIndex = d.nullableIndex;
        }
        return (*this);
    };

    //关系名称
    char relName[MAXNAME + 1];
    //属性名称
    char attrName[MAXNAME + 1];
    //属性的偏移
    int offset;
    //属性类型
    AttrType attrType;
    //属性的长度
    int attrSize;
    //属性的显示长度
    int attrDisplayLength;
    //属性规格（非空等）
    int attrSpecs;
    //属性的索引号
    int indexNo;
    //允许为空位图上的索引
    int nullableIndex;
};

//打印一些空格
void Spaces(int maxLength, int printedSoFar);


class Printer {
public:
    //构造函数
    Printer(const std::vector<DataAttrInfo> &attributes);
    //析构函数
    ~Printer();
    //打印header
    void PrintHeader(std::ostream &c) const;
    //打印footer
    void PrintFooter(std::ostream &c) const;
    
    //Print
    void Print(std::ostream &c, const char * const data, bool isnull[]);
    

private:
    //属性目录
    DataAttrInfo *attributes;
    //打印的属性数
    int attrCount;

    //标头信息的字符串数组
    char **psHeader;
    //每个属性之间的空格数
    int *spaces;

    //打印的元组数
    int iCount;
};


// #ifndef min
// #define min(a,b) (((a) < (b)) ? (a) : (b))
// #endif
//
// #ifndef max
// #define max(a,b) (((a) > (b)) ? (a) : (b))
// #endif

#endif
