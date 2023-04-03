#include <cstring>
#include <cstddef>
#include "../PF/pf.h"
#include "rm.h"
#include "rm_internal.h"

/* RM FileHandle */
RM_FileHandle::RM_FileHandle() {
    recordSize = 0;
}

RM_FileHandle::~RM_FileHandle() {}

/**
 * 根据RID,获得数据表的对应记录
 * @param rid 目标记录的RID
 * @param rec 返回的目标记录值
 * @return
 *         RM_FILE_NOT_OPENED       文件未打开
 *         RM_SLOTNUM_OUT_OF_RANGE  槽号超出范围
 *         OK_RC                    获得数据
 */
RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const {
    // 如果recordSize为0，表示数据表没有打开
    if (recordSize == 0) return RM_FILE_NOT_OPENED;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pageHandle;
    char *data;
    // 获得rid表示的记录的页号和槽号
    TRY(rid.GetPageNum(pageNum));
    TRY(rid.GetSlotNum(slotNum));
    // 判断槽号是否超出范围
    if (slotNum >= recordsPerPage || slotNum < 0)
        return RM_SLOTNUM_OUT_OF_RANGE;
    // 根据页号得到记录页
    TRY(pfHandle.GetThisPage(pageNum, pageHandle));
    // 得到记录的数据
    TRY(pageHandle.GetData(data));

    rec.rid = rid;
    // 将记录数据复制到rec的数据段
    rec.SetData(data + pageHeaderSize + recordSize * slotNum, (size_t)recordSize);

    // 设置rec的空值数组
    if (nullableNum > 0) {
        bool *isnull = new bool[nullableNum];
        for (int i = 0; i < nullableNum; ++i) {
            isnull[i] = getBitMap(((RM_PageHeader *)data)->bitmap,
                                  recordsPerPage + slotNum * nullableNum + i);
        }
        rec.SetIsnull(isnull, nullableNum);
    }

    // 调用个GetThis方法后要调用UnpinPage
    TRY(pfHandle.UnpinPage(pageNum));
    return OK_RC;
}

/**
 * 向数据表插入一条记录，返回这条记录的RID
 * @param pData 要插入记录的数据
 * @param rid   记录的RID
 * @param isnull 记录的空置数组
 * @return
 *        RM_FILE_NOT_OPENED   文件未打开
 *        OK_RC                插入成功
 *        其他                  其他错误
 *
 */
RC RM_FileHandle::InsertRec(const char *pData, RID &rid, bool *isnull) {
    // 判断文件是否被打开
    if (recordSize == 0) return RM_FILE_NOT_OPENED;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pageHandle;
    char *data, *destination;

    if (firstFreePage != kLastFreePage) {    // 还有空闲槽页

        // 得到首个空闲槽页，并得到这个页的页号和数据
        TRY(pfHandle.GetThisPage(firstFreePage, pageHandle));
        TRY(pageHandle.GetPageNum(pageNum));
        TRY(pageHandle.GetData(data));

        // 得到首个空闲槽的槽号
        slotNum = ((RM_PageHeader *)data)->firstFreeRecord;
        // 计算空闲槽的地址
        destination = data + pageHeaderSize + recordSize * slotNum;
        // 修改firstFreeRecord字段
        ((RM_PageHeader *)data)->firstFreeRecord = *(short *)destination;
        // 若是达到空闲槽尾
        if (*(short *)destination == kLastFreeRecord) {
            // 该页记录存满，将firstFreePage设置为nextFreePage
            firstFreePage = ((RM_PageHeader *)data)->nextFreePage;
            // 标记文件头被修改
            isHeaderDirty = true;
        }
    } else {    // 没有空闲槽页
        // 得到最后一个页
        TRY(pfHandle.GetLastPage(pageHandle));
        TRY(pageHandle.GetPageNum(pageNum));
        TRY(pageHandle.GetData(data));
        // 检查该页是否还有空间
        CHECK(((RM_PageHeader *)data)->allocatedRecords <= recordsPerPage);
        if (((RM_PageHeader *)data)->allocatedRecords == recordsPerPage) {  // 空间已经满了
            TRY(pfHandle.UnpinPage(pageNum));                           // 把这个页回收
            TRY(pfHandle.AllocatePage(pageHandle));                     // 申请一个新页
            TRY(pageHandle.GetPageNum(pageNum));
            TRY(pageHandle.GetData(data));
            *(RM_PageHeader *)data = {kLastFreeRecord, 0, kLastFreePage};       // 配置数据表页头
            memset(data + offsetof(RM_PageHeader, bitmap), 0,
                   (size_t)(recordsPerPage * (nullableNum + 1)));
        }
        // 计算记录可被插入的位置
        slotNum = ((RM_PageHeader *)data)->allocatedRecords;
        destination = data + pageHeaderSize + recordSize * slotNum;
        ++((RM_PageHeader *)data)->allocatedRecords;
    }
    // 插入记录
    memcpy(destination, pData, (size_t)recordSize);
    setBitMap(((RM_PageHeader *)data)->bitmap, slotNum, true);
    // 修改位图
    for (int i = 0; i < nullableNum; ++i) {
        setBitMap(((RM_PageHeader *)data)->bitmap,
                  recordsPerPage + slotNum * nullableNum + i, isnull[i]);
    }
    rid = RID(pageNum, slotNum);

    // 标记该页为脏页，unpin这个页
    TRY(pfHandle.MarkDirty(pageNum));
    TRY(pfHandle.UnpinPage(pageNum));
    return OK_RC;
}

/**
 * 根据所给的RID删除一条记录
 * 将位图对应位置设置为false，若该操作使一个满页变成空闲页
 * 需要将数据表头的firstFreePage字段设置为该页并标记数据表头被修改
 * @param rid 待删除记录的RID
 * @return
 */
RC RM_FileHandle::DeleteRec(const RID &rid) {
    // 检查文件是否打开
    if (recordSize == 0) return RM_FILE_NOT_OPENED;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pageHandle;
    char *data;
    // 目标页号和槽号
    TRY(rid.GetPageNum(pageNum));
    TRY(rid.GetSlotNum(slotNum));
    // 判断槽号是否超出范围
    if (slotNum >= recordsPerPage || slotNum < 0)
        return RM_SLOTNUM_OUT_OF_RANGE;
    // 得到这个页和数据
    TRY(pfHandle.GetThisPage(pageNum, pageHandle));
    TRY(pageHandle.GetData(data));

    // 该位置的记录本来就不存在，返回错误
    if (getBitMap(((RM_PageHeader *)data)->bitmap, slotNum) == 0)
        return RM_RECORD_DELETED;

    // 设置位图
    setBitMap(((RM_PageHeader *)data)->bitmap, slotNum, false);
    *(short *)(data + pageHeaderSize + recordSize * slotNum) = ((RM_PageHeader *)data)->firstFreeRecord;
    if (((RM_PageHeader *)data)->firstFreeRecord == kLastFreeRecord) {
        // 若该页从满页变成了空闲槽页，需要修改文件表头
        ((RM_PageHeader *)data)->nextFreePage = firstFreePage;
        firstFreePage = pageNum;
        isHeaderDirty = true;
    }
    // 修改数据表页头的firstFreeRecord字段为刚刚设置的槽号
    ((RM_PageHeader *)data)->firstFreeRecord = (short)slotNum;

    // 标记脏页并Unpin
    TRY(pfHandle.MarkDirty(pageNum));
    TRY(pfHandle.UnpinPage(pageNum));
    return OK_RC;
}

/**
 * 根据rec更新记录
 * 判断rec的数据是否有效，根据rec的rid读取的目标数据，修改数据
 * @param rec 新数据
 * @return
 */
RC RM_FileHandle::UpdateRec(const RM_Record &rec) {
    // 检查文件是否打开
    if (recordSize == 0) return RM_FILE_NOT_OPENED;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pageHandle;
    char *data;
    TRY(rec.rid.GetPageNum(pageNum));
    TRY(rec.rid.GetSlotNum(slotNum));
    if (slotNum >= recordsPerPage || slotNum < 0)
        return RM_SLOTNUM_OUT_OF_RANGE;
    TRY(pfHandle.GetThisPage(pageNum, pageHandle));
    TRY(pageHandle.GetData(data));

    // 更新数据
    memcpy(data + pageHeaderSize + recordSize * slotNum, rec.pData, (size_t)recordSize);
    for (int i = 0; i < nullableNum; ++i) {
        setBitMap(((RM_PageHeader *)data)->bitmap,
                  recordsPerPage + slotNum * nullableNum + i, rec.isnull[i]);
    }

    // 标记脏页并回收
    TRY(pfHandle.MarkDirty(pageNum));
    TRY(pfHandle.UnpinPage(pageNum));
    return OK_RC;
}

/**
 * 将数据表页的回写磁盘，调用pfHandle的ForcePage方法
 * @param pageNum 页号
 * @return
 */
RC RM_FileHandle::ForcePages(PageNum pageNum) {
    return pfHandle.ForcePages(pageNum);
}
