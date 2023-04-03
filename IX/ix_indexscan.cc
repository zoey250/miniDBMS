#include "ix.h"
#include "ix_internal.h"


#pragma mark - 1.构造函数&析构函数
//构造函数
IX_IndexScan::IX_IndexScan() {
    //初始化赋值
    scanOpened = false;
}

//析构函数
IX_IndexScan::~IX_IndexScan() { }


bool IX_IndexScan::__check(void* key) {
#define CMP_RESULT (indexHandle->__cmp(key, this->value))
    switch (compOp) {
        case GT_OP:
        case GE_OP:
            // initial search has done all the work
            return true;
        case EQ_OP:
            return CMP_RESULT == 0;
        case NE_OP:
            return CMP_RESULT != 0;
        case LT_OP:
            return CMP_RESULT < 0;
        case LE_OP:
            return CMP_RESULT <= 0;
        case NO_OP:
            return true;
        case ISNULL_OP:
        case NOTNULL_OP:
            LOG(FATAL) << "null is not supported in IX";
    }
}

#pragma mark - 2.打开/关闭扫描
/**
 此方法初始化一个 indexHandle 指代的索引的条件扫描器，其返回所有满足条件的记录的标识符
 参数indexHandle为打开的索引文件处理器
 参数compOp为比较器
 参数value为索引的键值
 参数pinHint为ClientHint
 */
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint pinHint) {
    //保证此函数调用时扫描器尚未打开
    if (scanOpened) {
        return IX_SCAN_NOT_CLOSED;
    }
    //如果打开的indexHanlde索引文件处理器有效
    //更新扫描器indexHandle属性指向输入参数中的indexHanlde
    this->indexHandle = &indexHandle;
    //设置比较器值
    this->compOp = compOp;
    this->value = value;
    bool initial_search_needed = false;
    //分支处理 调用不同的比较函数
    switch (compOp) {
        case GT_OP:
        case GE_OP:
            initial_search_needed = true;
            break;
        case NO_OP:
        case EQ_OP:
        case NE_OP:
        case LT_OP:
        case LE_OP:
            initial_search_needed = false;
            break;
        case ISNULL_OP:
        case NOTNULL_OP:
            LOG(FATAL) << "null is not supported in IX";
    }
    //从根结点开始
    currentNodeNum = indexHandle.root;
    currentEntryIndex = 0;
    currentBucketIndex = 0;
    
    const PF_FileHandle &file = indexHandle.pfHandle;
    bool should_stop = false;
    //遍历
    while (!should_stop) {
        PF_PageHandle page;
        IX_PageHeader *header;
        //获取currentNodeNum的页面
        TRY(file.GetThisPage(currentNodeNum, page));
        int openedPageNum = currentNodeNum;
        //获取currentNodeNum对应的页面的数据
        TRY(page.GetData(CVOID(header)));
        //如果是叶子结点
        if (header->type == kLeafNode) {
            if (compOp == GT_OP || compOp == GE_OP) {
                for (currentEntryIndex = 0; currentEntryIndex < header->childrenNum;
                        ++currentEntryIndex) {
                    Entry *entry = (Entry*)indexHandle.__get_entry(
                            header->entries, currentEntryIndex);
                    int c = indexHandle.__cmp(entry->key, value);
                    if ((compOp == GT_OP && c > 0) ||
                            (compOp == GE_OP && c >= 0)) {
                        break;
                    }
                }
            }
            should_stop = true;
        }
        //是内部结点
        else {
            int index = 0;
            if (compOp == GT_OP || compOp == GE_OP) {
                index = header->childrenNum - 1;
                for (int i = 0; i < header->childrenNum - 1; ++i) {
                    if (indexHandle.__cmp(((Entry*)indexHandle.__get_entry(
                                        header->entries, i))->key, value) > 0) {
                        index = i;
                        break;
                    }
                }
            }
            currentNodeNum = ((Entry*)indexHandle.__get_entry(
                        header->entries, index))->pageNum;
        }
        //取消固定
        TRY(file.UnpinPage(openedPageNum));
    }
    scanOpened = true;
    return 0;
}

/**
此方法终止索引扫描
参数无
*/
RC IX_IndexScan::CloseScan() {
    scanOpened = false;
    return 0;
}


#pragma mark - 2.获取下一条记录的RID
/**
 此方法把参数 rid 设置为索引扫描中下一条记录的标识符
 参数rid为给定的RID
 */
RC IX_IndexScan::GetNextEntry(RID &rid) {
    //如果扫描器并未打开，则返回IX_SCAN_NOT_OPENED
    if (!scanOpened) {
        return IX_SCAN_NOT_OPENED;
    }
    
    const PF_FileHandle &file = indexHandle->pfHandle;
    PF_PageHandle page;
    int ret = 0;
    //指示是否找到
    bool should_exit = false;
    //遍历
    while (!should_exit) {
        IX_PageHeader* header;
        //获取当前结点所在页面数据
        int openedPageNum = currentNodeNum;
        TRY(file.GetThisPage(currentNodeNum, page));
        TRY(page.GetData(CVOID(header)));
        Entry* entry = (Entry*)indexHandle->__get_entry(header->entries, currentEntryIndex);
        //如果当前项的索引值等于当前结点的键值数 即已经到当前索引块的结尾
        if (currentEntryIndex == header->childrenNum) {
            //如果下一个页面没有数据值
            if (entry->pageNum == kNullNode) {
                ret = IX_EOF;
                should_exit = true;
            }
            //下一页面仍然有键值 再次设置从0开始
            else {
                currentNodeNum = entry->pageNum;
                currentEntryIndex = 0;
                currentBucketIndex = 0;
            }
        }
        //没有到索引块结尾
        else {
            if (__check(entry->key)) {
                if (entry->pageNum == kInvalidBucket) {
                    ++currentEntryIndex;
                    currentBucketIndex = 0;
                }
                else {
                    PF_PageHandle bucket;
                    IX_BucketHeader *bucket_header;
                    //获取索引块的内容
                    TRY(file.GetThisPage(entry->pageNum, bucket));
                    TRY(bucket.GetData(CVOID(bucket_header)));
                    //找到当前索引块的最后一个键值
                    if (currentBucketIndex >= bucket_header->ridNum) {
                        ++currentEntryIndex;
                        currentBucketIndex = 0;
                    }
                    //没有找到
                    else {
                        rid = bucket_header->rids[currentBucketIndex];
                        ++currentBucketIndex;
                        should_exit = true;
                    }
                    //取消固定
                    TRY(file.UnpinPage(entry->pageNum));
                }
            }
            else {
                if (compOp == LT_OP || compOp == LE_OP) {
                    ret = IX_EOF;
                    should_exit = true;
                } else {
                    ++currentEntryIndex;
                    currentBucketIndex = 0;
                }
            }
        }
        TRY(file.UnpinPage(openedPageNum));
    }
    return ret;
}

