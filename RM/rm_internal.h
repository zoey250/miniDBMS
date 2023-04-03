#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

static const int kLastFreePage = -1;
static const int kLastFreeRecord = -2;

/**
 * 数据表页头
 */
struct RM_PageHeader {
    short firstFreeRecord;      // 记录的首个空闲槽号
    short allocatedRecords;     // 已经存储的记录数
    int nextFreePage;           // 下一个空闲页页号
    unsigned char bitmap[1];    // 位图字段
};

/**
 * 数据表表头
 */
struct RM_FileHeader {
    short recordSize;           // 记录长度
    short recordsPerPage;       // 每页的记录数
    short nullableNum;          // 空值数目
    int firstFreePage;          // 首个空闲槽页页号
    short nullableOffsets[1];
};

// 得到位图
inline bool getBitMap(unsigned char *bitMap, int pos) {
    return (bool)(bitMap[pos >> 3] >> (pos & 0x7) & 1);
}

// 设置位图
inline void setBitMap(unsigned char *bitMap, int pos, bool value) {
    if (value) {
        bitMap[pos >> 3] |= (unsigned char)(1 << (pos & 0x7));
    } else {
        bitMap[pos >> 3] &= (unsigned char)~(1 << (pos & 0x7));
    }
}

#endif //REBASE_RM_INTERNAL_H
