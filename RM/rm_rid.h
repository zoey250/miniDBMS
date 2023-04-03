//
// rm_rid.h
//
//   The Record Id interface
//

#ifndef RM_RID_H
#define RM_RID_H

#include "../redbase.h"

//
// PageNum: 页号
//
typedef int PageNum;

//
// SlotNum: 槽号
//
typedef int SlotNum;

//
// RID: RID是一条记录的唯一标识，记载了记录所在的页号和槽号
//
class RID {
public:
    RID();                                         // Default constructor
    RID(PageNum pageNum, SlotNum slotNum);
    ~RID();                                        // Destructor

    RC GetPageNum(PageNum &pageNum) const;         // 返回页号
    RC GetSlotNum(SlotNum &slotNum) const;         // 返回槽号

    inline bool operator==(const RID& r) const {
        return pageNum == r.pageNum && slotNum == r.slotNum;
    }

private:
    PageNum pageNum;    // 记录所在页号
    SlotNum slotNum;    // 记录所在槽号
};

#endif
