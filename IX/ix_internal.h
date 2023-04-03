#pragma once

#include "../redbase.h"
#include "../RM/rm_rid.h"

//常量定义
static const int kLastFreePage = -1;
static const int kNullNode = -1;
static const int kInvalidBucket = -1;

//索引文件头部信息
struct IX_FileHeader {
    //属性类型
    AttrType attrType;
    //属性长度
    int attrLength;
    //根结点
    int root;
    //第一个空闲的页面
    int firstFreePage;
};

//索引文件结点类型 内部结点or叶子结点
enum IX_NodeType {
    //内部结点
    kInternalNode,
    //叶子结点
    kLeafNode,
};

//索引页面的header
struct IX_PageHeader {
    //类型
    short type;
    //键值数
    short childrenNum;
    char entries[4];
};

//索引块页面的header
struct IX_BucketHeader {
    //索引块内保存的记录数
    int ridNum;
    RID rids[1];
};

//项信息
struct Entry {
    //内部结点项 pageNum是子结点的页码
    //叶子结点项 pageNum是对应的索引块的页码，除了最后一项指向的是下一个叶子结点的页码
    int pageNum;
    char key[4];
};



