#include <cstring>
#include <iostream>
#include "statistics.h"

using namespace std;

//PF层使用的统计信息键值
const char *PF_GETPAGE = "GETPAGE";
const char *PF_PAGEFOUND = "PAGEFOUND";
const char *PF_PAGENOTFOUND = "PAGENOTFOUND";
const char *PF_READPAGE = "READPAGE";
const char *PF_WRITEPAGE = "WRITEPAGE";
const char *PF_FLUSHPAGES = "FLUSHPAGES";

#pragma mark - Statistic类函数实现
//默认构造函数
Statistic::Statistic() {
    psKey = NULL;
    iValue = 0;
}

//StatisticMgr类使用的构造方法，保证psKey_不是NULL指针
Statistic::Statistic(const char *psKey_) {
    psKey = new char[strlen(psKey_) + 1];
    strcpy (psKey, psKey_);

    iValue = 0;
}

//复制构造函数
Statistic::Statistic(const Statistic &stat) {
    psKey = new char[strlen(stat.psKey) + 1];
    strcpy (psKey, stat.psKey);

    iValue = stat.iValue;
}

//平等构造函数
Statistic& Statistic::operator=(const Statistic &stat) {
    if (this == &stat)
        return *this;

    delete [] psKey;
    psKey = new char[strlen(stat.psKey) + 1];
    strcpy (psKey, stat.psKey);

    iValue = stat.iValue;

    return *this;
}

//析构函数
Statistic::~Statistic() {
    delete [] psKey;
}

//判断等价
Boolean Statistic::operator==(const char *psKey_) const {
    return (strcmp(psKey_, psKey) == 0);
}

#pragma mark - StatisticMgr类函数实现
//注册对统计信息的更改
RC StatisticsMgr::Register (const char *psKey, const Stat_Operation op,
                            const int *const piValue) {
    int i, iCount;
    Statistic *pStat = NULL;

    if (psKey == NULL || (op != STAT_ADDONE && piValue == NULL))
        return STAT_INVALID_ARGS;

    iCount = llStats.GetLength();

    for (i = 0; i < iCount; i++) {
        pStat = llStats[i];
        if (*pStat == psKey)
            break;
    }

    //检查是否找到了统计信息
    if (i == iCount)
        //尚未找到，因此创建一个新的统计信息（键为psKey，初始值为0）
        pStat = new Statistic( psKey );

    //对统计信息执行操作
    switch (op) {
    case STAT_ADDONE:
        pStat->iValue++;
        break;
    case STAT_ADDVALUE:
        pStat->iValue += *piValue;
        break;
    case STAT_SETVALUE:
        pStat->iValue = *piValue;
        break;
    case STAT_MULTVALUE:
        pStat->iValue *= *piValue;
        break;
    case STAT_DIVVALUE:
        pStat->iValue = (int) (pStat->iValue / (*piValue));
        break;
    case STAT_SUBVALUE:
        pStat->iValue -= *piValue;
        break;
    };

    //如果该统计信息不在原始列表中，则将其添加到名单中
    if (i == iCount) {
        llStats.Append(*pStat);
        delete pStat;
    }

    return 0;
}

//打印与特定统计信息有关的信息
RC StatisticsMgr::Print(const char *psKey) {
    if (psKey == NULL)
        return STAT_INVALID_ARGS;

    int *iValue = Get(psKey);

    if (iValue)
        cout << psKey << "::" << *iValue << "\n";
    else
        return STAT_UNKNOWN_KEY;

    delete iValue;

    return 0;
}


//返回一个指向关联的整数值的指针，具有特定的统计信息。 如果找不到统计信息，则它将返回NULL
int *StatisticsMgr::Get(const char *psKey) {
    int i, iCount;
    Statistic *pStat = NULL;

    iCount = llStats.GetLength();

    for (i = 0; i < iCount; i++) {
        pStat = llStats[i];
        if (*pStat == psKey)
            break;
    }

    //检查是否找到了统计信息
    if (i == iCount)
        return NULL;

    return new int(pStat->iValue);
}

//打印所有跟踪的统计信息
void StatisticsMgr::Print() {
    int i, iCount;
    Statistic *pStat = NULL;

    iCount = llStats.GetLength();

    for (i = 0; i < iCount; i++) {
        pStat = llStats[i];
        cout << pStat->psKey << "::" << pStat->iValue << "\n";
    }
}

//重置特定的统计信息
RC StatisticsMgr::Reset(const char *psKey) {
    int i, iCount;
    Statistic *pStat = NULL;

    if (psKey == NULL)
        return STAT_INVALID_ARGS;

    iCount = llStats.GetLength();

    for (i = 0; i < iCount; i++) {
        pStat = llStats[i];
        if (*pStat == psKey)
            break;
    }

    //如果找到了统计信息，则将其从列表中删除
    if (i != iCount)
        llStats.Delete(i);
    else
        return STAT_UNKNOWN_KEY;

    return 0;
}

//重置所有统计信息
void StatisticsMgr::Reset() {
    llStats.Erase();
}
