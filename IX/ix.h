//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H


#include "../redbase.h"
#include "rm_rid.h"
#include "../PF/pf.h"
#include "../RM/rm.h"

#include <memory>
#include <glog/logging.h>

class IX_IndexHandle;

//提供索引文件管理
class IX_Manager {
    //与该索引关联的PF_Manager
    PF_Manager *pfm;
    // RM_Manager rmm;

public:
    //构造函数
    IX_Manager   (PF_Manager &pfm);
    //析构函数
    ~IX_Manager  ();
    //创建一个新索引
    RC CreateIndex  (const char *fileName,
                     int        indexNo,
                     AttrType   attrType,
                     int        attrLength);
    //销毁索引
    RC DestroyIndex (const char *fileName,
                     int        indexNo);
    //打开索引
    RC OpenIndex    (const char *fileName,
                     int        indexNo,
                     IX_IndexHandle &indexHandle);
    //关闭索引
    RC CloseIndex   (IX_IndexHandle &indexHandle);
};

//索引文件处理接口
class IX_IndexHandle {
    //友元类
    friend class IX_Manager;
    friend class IX_IndexScan;

    // 与此索引关联的PF_FileHandle
    PF_FileHandle pfHandle;
    // RM_FileHandle rmHandle;

    //属性类型
    AttrType attrType;
    //属性长度
    int attrLength;
    //根节点
    int root;
    //第一个空闲的页
    int firstFreePage;
    //索引块
    int ridsPerBucket;
    // branch factor
    int b;
    int entrySize;
    
    //header是否被修改过
    bool isHeaderDirty;
    

    //使用属性类型和长度计算内部参数
    void __initialize();


    //比较器
    int __cmp(void* lhs, void* rhs) const;
    //获取项
    inline void* __get_entry(void* base, int n) const {
        return (void*)((char*)base + entrySize * n);
    }

    //创建新的结点
    RC new_node(int *nodeNum);
    //删除结点
    RC delete_node(int nodeNum);

    //创建新的索引块
    RC new_bucket(int *pageNum);
    //删除索引块
    RC delete_bucket(int pageNum);
    //索引块中的插入
    RC bucket_insert(int *pageNum,
                     const RID &rid);
    //索引块中的删除
    RC bucket_delete(int *pageNum,
                     const RID &rid);

    //插入内部项
    RC insert_internal_entry(void *header,
                             int index,
                             void* key,
                             int node);
    //插入项
    RC insert_entry(void *header,
                    void* pData,
                    const RID &rid);
    
    //插入内部结点
    RC insert_internal(int nodeNum,
                       int *splitNode,
                       std::unique_ptr<char[]> *splitKey,
                       void *pData,
                       const RID &rid);
    
    //插入叶子结点
    RC insert_leaf(int nodeNum,
                   int *splitNode,
                   void *pData,
                   const RID &rid);

public:
    //构造函数
    IX_IndexHandle  ();
    //析构函数
    ~IX_IndexHandle ();
    //插入新的索引项
    RC InsertEntry     (void *pData,
                        const RID &rid);
    
    //删除索引项
    RC DeleteEntry     (void *pData,
                        const RID &rid);
    
    //将索引文件写会磁盘
    RC ForcePages      ();

    RC Traverse(int nodeNum = 0,
                int depth = 0);
};

//基于条件的索引项扫描
class IX_IndexScan {
    //指向indexHandle的指针，该指针修改扫描将尝试遍历的文件
    const IX_IndexHandle *indexHandle;
    //比较类型，以及有关要比较的值的信息
    CompOp compOp;
    void* value;
    
    //指示是否正在使用扫描的指针
    bool scanOpened;
    //当前结点数
    int currentNodeNum;
    //当前项索引
    int currentEntryIndex;
    //当前索引块索引
    int currentBucketIndex;
    
    //验证函数
    bool __check(void* key);

public:
    //构造函数
    IX_IndexScan  ();
    //析构函数
    ~IX_IndexScan ();
    //打开索引扫描
    RC OpenScan      (const IX_IndexHandle &indexHandle,
                      CompOp      compOp,
                      void        *value,
                      ClientHint  pinHint = NO_HINT);
    //关闭索引扫描
    RC CloseScan     ();
    //获取下一个匹配的项，如果没有更多匹配的条目，返回IX_EOF
    RC GetNextEntry  (RID &rid);
};

//打印错误功能
void IX_PrintError(RC rc);

#define IX_EOF                  (START_IX_WARN + 0)
#define IX_ENTRY_EXISTS         (START_IX_WARN + 1)
#define IX_ENTRY_DOES_NOT_EXIST (START_IX_WARN + 2)
#define IX_SCAN_NOT_OPENED      (START_IX_WARN + 3)
#define IX_SCAN_NOT_CLOSED      (START_IX_WARN + 4)
#define IX_BUCKET_FULL          (START_IX_WARN + 5)
#define IX_LASTWARN IX_SCAN_NOT_CLOSED


#define IX_ATTR_TOO_LARGE       (START_IX_ERR - 0)
#define IX_LASTERROR IX_ATTR_TOO_LARGE

#endif // IX_H
