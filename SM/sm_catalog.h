#ifndef MICRODBMS_CATALOG_H
#define MICRODBMS_CATALOG_H

#include "../redbase.h"

//定义关系的目录项
struct RelCatEntry {
    //关系名
    char relName[MAXNAME + 1];
    //元组长度
    int tupleLength;
    //属性数量
    int attrCount;
    //索引数量
    int indexCount;
    //关系表中记录数
    int recordCount;
};

//定义属性的目录项
struct AttrCatEntry {
    //关系名
    char relName[MAXNAME + 1];
    //属性名
    char attrName[MAXNAME + 1];
    //元组其实的字节偏移量
    int offset;
    //属性类型
    AttrType attrType;
    //属性大小 （对齐）
    int attrSize;
    //属性显示长度
    int attrDisplayLength;
    //属性规格 （非空限制等）
    int attrSpecs;
    //索引标号，-1为没有索引
    int indexNo;
};

#endif //MICRODBMS_CATALOG_H
