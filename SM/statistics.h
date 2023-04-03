#ifndef STATISTICS_H
#define STATISTICS_H

//一些可能尚未设置的定义
#ifndef Boolean
typedef char Boolean;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef RC
typedef int RC;
#endif

#ifndef STAT_BASE
const int STAT_BASE = 9000;
#endif

//链接列表的模板类
#include "linkedlist.h"

//统计信息（动态跟踪客户端的统计信息）
class Statistic {
public:
    //构造函数
    Statistic();
    Statistic(const char *psName);
    //析构函数
    ~Statistic();
    
    //复制构造函数
    Statistic(const Statistic &stat);

    //平等构造函数
    Statistic& operator=(const Statistic &stat);

    //检查统计信息和名称之间是否相等
    Boolean operator==(const char *psName_) const;

    //统计信息的名称或关键字
    char *psKey;

    //允许统计信息跟踪整数值，初始值为0。
    int iValue;
};

//统计信息可以执行的不同操作的定义
enum Stat_Operation {
    STAT_ADDONE,
    STAT_ADDVALUE,
    STAT_SETVALUE,
    STAT_MULTVALUE,
    STAT_DIVVALUE,
    STAT_SUBVALUE
};


//跟踪一组统计信息
class StatisticsMgr {

public:
    //构造函数
    StatisticsMgr() {};
    //析构函数
    ~StatisticsMgr() {};

    //添加新的统计信息或将更改注册到现有统计信息
    RC Register(const char *psKey, const Stat_Operation op,
                const int *const piValue = NULL);

    //返回与特定统计信息关联的值
    int *Get(const char *psKey);

    //打印特定的统计信息
    RC Print(const char *psKey);

    //打印所有跟踪的统计信息
    void Print();

    //重置特定的统计信息
    RC Reset(const char *psKey);

    //重置所有统计信息
    void Reset();

private:
    LinkList<Statistic> llStats;
};

//返回码
//方法调用中的错误Args
const int STAT_INVALID_ARGS = STAT_BASE+1;
//没有跟踪到这样的密钥
const int STAT_UNKNOWN_KEY  = STAT_BASE+2;

//用作统计信息管理器的键的一些常量
extern const char *PF_GETPAGE;
extern const char *PF_PAGEFOUND;
extern const char *PF_PAGENOTFOUND;
extern const char *PF_READPAGE;
extern const char *PF_WRITEPAGE;
extern const char *PF_FLUSHPAGES;

#endif

