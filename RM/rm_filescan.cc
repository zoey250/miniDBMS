#include "rm.h"
#include "rm_internal.h"

#include <cassert>

RM_FileScan::RM_FileScan() {
    scanOpened = false;
}

RM_FileScan::~RM_FileScan() {}

/**
 * 对fileHandle的数据表创建条件查询器
 * @param fileHandle    数据表
 * @param attrType      条件属性
 * @param attrLength    属性字节数
 * @param attrOffset    属性便宜
 * @param compOp        比较操作符
 * @param value         属性条件值
 * @param pinHint       允许多个pin
 * @return
 */
RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, void *value, ClientHint pinHint) {
    // 若查询器已经打开，返回错误
    if (scanOpened) return RM_SCAN_NOT_CLOSED;
    // 配置查询器的全局变量
    this->fileHandle = &fileHandle;
    this->attrType = attrType;
    this->attrLength = attrLength;
    this->attrOffset = attrOffset;
    this->compOp = compOp;
    if (value == NULL) {
        this->value.stringVal = NULL;
    } else {
        switch (attrType) { // 根据属性类型转换查询条件值
            case INT:
                this->value.intVal = *(int *)value;
                break;
            case FLOAT:
                this->value.floatVal = *(float *)value;
                break;
            case STRING:
                this->value.stringVal = new char[attrLength + 1];
                memcpy(this->value.stringVal, value, (size_t)attrLength);
                this->value.stringVal[attrLength] = '\0';
                break;
        }
    }

    RM_FileHeader *fileHeader;
    PF_PageHandle pageHandle;
    // 设置查询打开标志为true
    scanOpened = true;
    // 获取文件的第一页和数据，也就是页号为-1的下一个有效页，一般是0号页，用来存放数据表的格式信息
    TRY(fileHandle.pfHandle.GetFirstPage(pageHandle));
    TRY(pageHandle.GetData(CVOID(fileHeader)));
    recordSize = fileHeader->recordSize;
    nullableIndex = -1;
    int nullableNum = fileHeader->nullableNum;
    // 计算空值字段的位置
    for (int i = 0; i < nullableNum; ++i) {
        if (fileHeader->nullableOffsets[i] == attrOffset) {
            VLOG(2) << "nullableIndex = " << i;
            nullableIndex = i;
            break;
        }
    }
    if (nullableIndex == -1) {
        VLOG(3) << "given offset was not found to be a nullable field.";
    }
    // 设置查询起始页的页号为1,槽号为0，
    currentPageNum = 1;
    currentSlotNum = 0;
    // 这里应该是UnpinPage的页号是pageHandle的页号
    PageNum pageNum;
    TRY(pageHandle.GetPageNum(pageNum))
//    TRY(fileHandle.pfHandle.UnpinPage(0));
    TRY(fileHandle.pfHandle.UnpinPage(pageNum));

    return OK_RC;
}


/**
 * 根据currentPageNum和currentSlotNum得到下一个满足条件的记录
 * 每回都从currentSlotNum开始查询，因此，在最后要++currentSlotNum
 * 构建RM_Record返回
 * @param rec 满足条件的记录
 * @return
 */
RC RM_FileScan::GetNextRec(RM_Record &rec) {
    // 检查扫描器是否打开
    if (!scanOpened) return RM_SCAN_NOT_OPENED;

    char *data;
    PF_PageHandle pageHandle;
    bool found = false;
    // 获得当前页
    TRY(fileHandle->pfHandle.GetThisPage(currentPageNum, pageHandle));
    while (true) {
        TRY(pageHandle.GetData(data));
        int cnt = ((RM_PageHeader *)data)->allocatedRecords;
        unsigned char *bitMap = ((RM_PageHeader *)data)->bitmap;
        for (; currentSlotNum < cnt; ++currentSlotNum) {
            if (getBitMap(bitMap, currentSlotNum) == 0) continue;
            char *pData = data + fileHandle->pageHeaderSize + recordSize * currentSlotNum;
            bool isnull = false;
            if (nullableIndex != -1) {
                isnull = getBitMap(((RM_PageHeader *)data)->bitmap,
                                   fileHandle->recordsPerPage +
                                   currentSlotNum * fileHandle->nullableNum + nullableIndex);
            }
            // 如果数据满足条件，设置found==true,退出循环
            if (checkSatisfy(pData, isnull)) {
                found = true;
                break;
            }
        }
        // 把当前页回收
        TRY(fileHandle->pfHandle.UnpinPage(currentPageNum));
        // 如果找到了满足条件的记录则推出循环
        if (found) break;
        // 没有找到，说明当前页检索完全，需要调取下一页
        int rc = fileHandle->pfHandle.GetNextPage(currentPageNum, pageHandle);
        if (rc == PF_EOF) return RM_EOF;
        else if (rc != 0) return rc;
        // 重新设置currentPageNum的值，和currentSlotNum为0,从每页的开始检索
        TRY(pageHandle.GetPageNum(currentPageNum));
        currentSlotNum = 0;
    }

    TRY(fileHandle->GetRec(RID(currentPageNum, currentSlotNum), rec));
    ++currentSlotNum;
    return OK_RC;
}

/**
 * 关闭查询器
 * 将scanOpend设置为false，回收内存
 * @return
 */
RC RM_FileScan::CloseScan() {
    if (!scanOpened) return RM_SCAN_NOT_OPENED;
    scanOpened = false;
    if (attrType == STRING && value.stringVal != NULL)
        delete[] value.stringVal;
    return 0;
}

/**
 * 判断一个属性值是否满足查询条件
 * @param data      数据
 * @param isnull    是否可以为空值
 * @return
 */
bool RM_FileScan::checkSatisfy(char *data, bool isnull) {
    if (compOp == NO_OP) return true;
    if (compOp == ISNULL_OP) {
        return isnull;
    }
    if (compOp == NOTNULL_OP) {
        return !isnull;
    }
    if (isnull) return false;
    switch (attrType) {
        case INT: {
            int curVal = *(int *)(data + attrOffset);
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return curVal == value.intVal;
                case NE_OP:
                    return curVal != value.intVal;
                case LT_OP:
                    return curVal < value.intVal;
                case GT_OP:
                    return curVal > value.intVal;
                case LE_OP:
                    return curVal <= value.intVal;
                case GE_OP:
                    return curVal >= value.intVal;
                default:
                    CHECK(false);
            }
        }
        case FLOAT: {
            float curVal = *(float *)(data + attrOffset);
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return curVal == value.floatVal;
                case NE_OP:
                    return curVal != value.floatVal;
                case LT_OP:
                    return curVal < value.floatVal;
                case GT_OP:
                    return curVal > value.floatVal;
                case LE_OP:
                    return curVal <= value.floatVal;
                case GE_OP:
                    return curVal >= value.floatVal;
                default:
                    CHECK(false);
            }
        }
        case STRING: {
            char *curVal = data + attrOffset;
            switch (compOp) {
                case NO_OP:
                    return true;
                case EQ_OP:
                    return strcmp(curVal, value.stringVal) == 0;
                case NE_OP:
                    return strcmp(curVal, value.stringVal) != 0;
                case LT_OP:
                    return strcmp(curVal, value.stringVal) < 0;
                case GT_OP:
                    return strcmp(curVal, value.stringVal) > 0;
                case LE_OP:
                    return strcmp(curVal, value.stringVal) <= 0;
                case GE_OP:
                    return strcmp(curVal, value.stringVal) >= 0;
                default:
                    CHECK(false);
            }
        }
    }
    assert(0);
    return false;
}
