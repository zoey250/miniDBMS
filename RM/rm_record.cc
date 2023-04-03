#include "rm.h"

RM_Record::RM_Record() {
    pData = NULL;
    isnull = NULL;
}

RM_Record::~RM_Record() {
    if (pData != NULL) {
        delete[] pData;
    }
    if (isnull != NULL) {
        delete[] isnull;
    }
}

/**
 * 重写记录的数据
 * @param data 重写的数据
 * @param size 数据字节数
 */
void RM_Record::SetData(char *data, size_t size) {
    if (pData != NULL) {
        delete[] pData;
    }
    pData = new char[size];
    memcpy(pData, data, size);
}

/**
 * 设置空置数组
 * @param isnull        空置数组
 * @param nullableNum   数组长度
 */
void RM_Record::SetIsnull(bool *isnull, short nullableNum) {
    if (this->isnull != NULL) {
        delete[] this->isnull;
    }
    this->isnull = new bool[nullableNum];
    memcpy(this->isnull, isnull, nullableNum * sizeof(bool));
}

/**
 * 返回记录数据
 * @param pData 数据
 * @return
 *         RM_UNINITIALIZED_RECORD 无效数据
 *         OK_RC                   成功获取数据
 */
RC RM_Record::GetData(char *&pData) const {
    pData = this->pData;
    return pData == NULL ? RM_UNINITIALIZED_RECORD : OK_RC;
}

/**
 * 获取空值字段数据
 * @param isnull  空值字段数组
 * @return
 *         RM_UNINITIALIZED_RECORD 无效数据
 *         OK_RC                   成功获取数据
 */
RC RM_Record::GetIsnull(bool *&isnull) const {
    isnull = this->isnull;
    return pData == NULL ? RM_UNINITIALIZED_RECORD : OK_RC;
}

/**
 * 获取数据的RID
 * @param rid  数据标识
 * @return
 *         RM_UNINITIALIZED_RECORD 无效数据
 *         OK_RC                   成功获取数据
 */
RC RM_Record::GetRid(RID &rid) const {
    rid = this->rid;
    return pData == NULL ? RM_UNINITIALIZED_RECORD : 0;
}
