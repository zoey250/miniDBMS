//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "../redbase.h"
#include "rm_rid.h"
#include "../PF/pf.h"

#include <glog/logging.h>
#include <cstring>

//
// RM_Record: 记录类
//
class RM_Record {
    friend class RM_FileHandle;
    friend class RM_FileScan;

    char *pData;        //记录的数据内容
    bool *isnull;       //记录空值数组，标记某个字段是否可以为空值
    RID rid;            //记录的RID
public:
    RM_Record ();
    RM_Record(const RM_Record&) = delete;
    ~RM_Record();

    RM_Record& operator=(const RM_Record&) = delete;

    void SetData(char *data, size_t size);                  // 设置记录数据
    void SetIsnull(bool* isnull, short nullableNum);        // 设置记录某个字段为空

    RC GetData(char *&pData) const;                         // 获取记录内容
    RC GetIsnull(bool *&pData) const;                       // 获取记录内容是否为空

    RC GetRid (RID &rid) const;                             // 得到记录的RID
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
    friend class RM_Manager;
    friend class RM_FileScan;

    PF_FileHandle pfHandle;         // 数据表关联的页式文件处理类
    short recordSize;               // 数据表的记录字节数
    short recordsPerPage;           // 每页的记录数
    int firstFreePage;              // 首个有空闲槽页的页号
    short pageHeaderSize;           // 文件头字节数

    short nullableNum;              // 空值数
    short* nullableOffsets;         // 空值数组偏移

    bool isHeaderDirty;             // 数据表头是否修改标识
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    RC GetRec     (const RID &rid, RM_Record &rec) const;               // 根据RID，获得数据表的记录

    RC InsertRec  (const char *pData, RID &rid, bool *isnull = NULL);   // 向数据表插入一条记录，返回数据的RID

    RC DeleteRec  (const RID &rid);                                     // 删除一条记录

    RC UpdateRec  (const RM_Record &rec);                               // 更新一条记录

    RC ForcePages (PageNum pageNum = ALL_PAGES);                        // 根据页号，将数据表的页回写磁盘
};

//
// RM_FileScan: 根据条件进行查询的迭代器
//
class RM_FileScan {
    const RM_FileHandle *fileHandle;    // 对应的数据表
    AttrType attrType;                  // 查询的数据类型
    int attrLength;                     // 属性字节数
    int attrOffset;                     // 属性的偏移
    CompOp compOp;                      // 比较操作值

    union {
        int intVal;
        float floatVal;
        char *stringVal;
    } value;                            // 属性类型

    bool scanOpened;                    // 扫描器打开标识
    PageNum currentPageNum;             // 当前的页号
    SlotNum currentSlotNum;             // 当前的槽号
    short recordSize;                   // 记录字节数
    int nullableIndex;                  // 空值索引

    // 条件是否满足
    bool checkSatisfy(char *data, bool isnull);
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    // 创建迭代器
    RC OpenScan  (const RM_FileHandle &fileHandle,  // 数据表
                  AttrType   attrType,              // 属性类型
                  int        attrLength,            // 属性字节数
                  int        attrOffset,            // 属性字段偏移
                  CompOp     compOp,                // 比较操作符
                  void       *value,                // 条件值
                  ClientHint pinHint = NO_HINT);
    // 迭代器
    RC GetNextRec(RM_Record &rec);                  // 得到下一个满足条件的记录

    // 关闭迭代器
    RC CloseScan ();                                 // 关闭迭代器
};

//
// RM_Manager: 数据表管理类，提供数据表的创建、销毁、打开和关闭的操作
//
class RM_Manager {
    PF_Manager *pfm;                                // 页式文件系统的指针，用来调用文件系统的接口
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName,            // 创建数据表
            int recordSize,
            short nullableNum = 0,
            short *nullableOffsets = NULL);

    RC DestroyFile(const char *fileName);           // 销毁数据表

    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);    // 打开数据表

    RC CloseFile  (RM_FileHandle &fileHandle);                          // 关闭数据表
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_FILE_NOT_OPENED      (START_RM_WARN + 0) // 文件没有打开
#define RM_SLOTNUM_OUT_OF_RANGE (START_RM_WARN + 1) // SlotNum < 0 || >= records/page
#define RM_RECORD_DELETED       (START_RM_WARN + 2) // 记录已经被删除
#define RM_EOF                  (START_RM_WARN + 3) // 数据表结尾
#define RM_UNINITIALIZED_RECORD (START_RM_WARN + 4) // 记录没有初始化
#define RM_UNINITIALIZED_RID    (START_RM_WARN + 5) // RID没有初始化
#define RM_SCAN_NOT_OPENED      (START_RM_WARN + 6) // 扫描器未打开
#define RM_SCAN_NOT_CLOSED      (START_RM_WARN + 7) // 扫描器没有关闭
#define RM_LASTWARN             RM_SCAN_NOT_CLOSED

#define RM_RECORDSIZE_TOO_LARGE (START_RM_ERR - 0) // 一条记录太长
#define RM_BAD_NULLABLE_NUM     (START_RM_ERR - 1) // 空值数量超出范围
#define RM_LASTERROR            RM_BAD_NULLABLE_NUM

#endif
