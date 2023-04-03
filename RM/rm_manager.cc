#include "../PF/pf.h"
#include "rm.h"
#include "rm_internal.h"

#include <stddef.h>

/* RM Manager */
RM_Manager::RM_Manager(PF_Manager &pfm) {
    this->pfm = &pfm;
}

RM_Manager::~RM_Manager() {}

/**
 * 利用pfm的createFile方法在文件系统中创建一个文件
 * 打开这个文件，并给它分配一个页，将数据表的格式信息写入到这个页
 * @param fileName          数据表名
 * @param recordSize        数据表的一条记录的字节数
 * @param nullableNum       记录空值的数目
 * @param nullableOffsets   空值数组
 * @return
 */
RC RM_Manager::CreateFile(const char *fileName, int recordSize,
                          short nullableNum, short *nullableOffsets) {
    // 检查记录的长度是否超过限制
    if (recordSize > PF_PAGE_SIZE) {
        return RM_RECORDSIZE_TOO_LARGE;
    }
    if (sizeof(RM_PageHeader) + nullableNum * sizeof(short) > PF_PAGE_SIZE) {
        return RM_RECORDSIZE_TOO_LARGE;
    }

    // 调用文件系统的接口创建数据表对应的文件
    pfm->CreateFile(fileName);

    PF_FileHandle fileHandle;
    PF_PageHandle pageHandle;
    RM_FileHeader *fileHeader;
    RM_PageHeader *pageHeader;

    // 打开文件，并分配一个页
    TRY(pfm->OpenFile(fileName, fileHandle));
    TRY(fileHandle.AllocatePage(pageHandle));
    TRY(pageHandle.GetData(CVOID(fileHeader)));

    //  size = sizeof PageHeader + bitmap[ = records * (1 + nullable)] +
    // records * recordSize
    short recordsPerPage = (PF_PAGE_SIZE - sizeof(RM_PageHeader)) /
                           (recordSize + nullableNum + 1);
    if (upper_align<4>(recordsPerPage * (nullableNum + 1)) +
        sizeof(RM_PageHeader) + recordSize * recordsPerPage > PF_PAGE_SIZE)
        --recordsPerPage;
    fileHeader->recordSize = (short)recordSize;
    fileHeader->recordsPerPage = recordsPerPage;
    fileHeader->nullableNum = nullableNum;
    fileHeader->firstFreePage = kLastFreePage;
    for (int i = 0; i < nullableNum; ++i) {
        fileHeader->nullableOffsets[i] = nullableOffsets[i];
    }

    // 标记脏页，并将数据格式页标记为脏页并unpin
    PageNum pageNum;
    TRY(pageHandle.GetPageNum(pageNum));
    TRY(fileHandle.MarkDirty(pageNum));
    TRY(fileHandle.UnpinPage(pageNum));
//    TRY(fileHandle.MarkDirty(0));
//    TRY(fileHandle.UnpinPage(0));

    // 申请一个页用于插入数据记录
    TRY(fileHandle.AllocatePage(pageHandle));
    TRY(pageHandle.GetData(CVOID(pageHeader)));

    // 配置数据页的页头信息
    *pageHeader = {kLastFreeRecord, 0, kLastFreePage};
    memset(pageHeader + offsetof(RM_PageHeader, bitmap), 0,
           (size_t)(recordsPerPage * (nullableNum + 1)));

    // 标记为脏页并unpin
//    TRY(fileHandle.MarkDirty(1));
//    TRY(fileHandle.UnpinPage(1));
    TRY(pageHandle.GetPageNum(pageNum));
    TRY(fileHandle.MarkDirty(pageNum));
    TRY(fileHandle.UnpinPage(pageNum));

    // 关闭文件
    TRY(pfm->CloseFile(fileHandle));
    return OK_RC;
}

/**
 * 删除数据表，删除数据表之前请确保数据表已经被关闭
 * 调用pfm的DestoryFile方法
 * @param fileName  数据表名
 * @return
 */
RC RM_Manager::DestroyFile(const char *fileName) {
    return pfm->DestroyFile(fileName);
}

/**
 * 打开数据表，得到数据表的句柄
 * @param fileName      数据表名
 * @param fileHandle    数据表处理类句柄
 * @return
 */
RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
    PF_FileHandle pfHandle;
    // 根据表名打开文件
    TRY(pfm->OpenFile(fileName, pfHandle));
    fileHandle.pfHandle = pfHandle;
    PF_PageHandle pageHandle;
    RM_FileHeader *data;
    // 得到文件的格式配置页
    TRY(pfHandle.GetFirstPage(pageHandle));
    TRY(pageHandle.GetData(CVOID(data)));

    //读取配置页的数据
    fileHandle.recordSize = data->recordSize;
    fileHandle.recordsPerPage = data->recordsPerPage;
    fileHandle.nullableNum = data->nullableNum;
    if (data->nullableNum < 0) {
        return RM_BAD_NULLABLE_NUM;
    } else if (data->nullableNum > 0) {
        fileHandle.nullableOffsets = new short[data->nullableNum];
    } else {
        fileHandle.nullableOffsets = NULL;
    }
    for (int i = 0; i < data->nullableNum; ++i) {
        fileHandle.nullableOffsets[i] = data->nullableOffsets[i];
    }
    fileHandle.firstFreePage = data->firstFreePage;
    fileHandle.isHeaderDirty = false;
    fileHandle.pageHeaderSize = sizeof(RM_PageHeader) + upper_align<4>(
            data->recordsPerPage * (1 + data->nullableNum));

    // 结束unpinPage
//    TRY(pfHandle.UnpinPage(0));
    PageNum pageNum;
    TRY(pageHandle.GetPageNum(pageNum));
    TRY(pfHandle.UnpinPage(pageNum));
    return OK_RC;
}

/**
 * 关闭数据表
 * @param fileHandle 数据表句柄
 * @return
 */
RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
    // 如果数据表头被修改，需要更新配置页的数据
    if (fileHandle.isHeaderDirty) {
        PF_PageHandle pageHandle;
        RM_FileHeader *header;
        TRY(fileHandle.pfHandle.GetFirstPage(pageHandle));
        TRY(pageHandle.GetData(CVOID(header)));

        header->recordSize = fileHandle.recordSize;
        header->recordsPerPage = fileHandle.recordsPerPage;
        header->nullableNum = fileHandle.nullableNum;
        header->firstFreePage = fileHandle.firstFreePage;
        for (int i = 0; i < fileHandle.nullableNum; ++i) {
            header->nullableOffsets[i] = fileHandle.nullableOffsets[i];
        }
        delete[] fileHandle.nullableOffsets;

//        TRY(fileHandle.pfHandle.MarkDirty(0));
//        TRY(fileHandle.pfHandle.UnpinPage(0));
        PageNum pageNum;
        TRY(pageHandle.GetPageNum(pageNum));
        TRY(fileHandle.pfHandle.MarkDirty(pageNum));
        TRY(fileHandle.pfHandle.UnpinPage(pageNum));
    }
    // 调用文件系统的CloseFile方法关闭文件
    TRY(pfm->CloseFile(fileHandle.pfHandle));
    return OK_RC;
}
