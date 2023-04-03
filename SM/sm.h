#ifndef SM_H
#define SM_H


#include <stdlib.h>
#include <string.h>
#include "../redbase.h"
#include "../RM/rm.h"
#include "../IX/ix.h"
#include <string>
#include <map>
#include "../PARSER/parser.h"
#include "sm_catalog.h"
#include "printer.h"

//数据管理
class SM_Manager {
    //友元类
    friend class QL_Manager;
    
    //记录文件管理器
    RM_Manager *rmm;
    //索引文件管理器
    IX_Manager *ixm;
    
    //relcat记录文件接口和attrcat记录文件接口
    RM_FileHandle relcat, attrcat;
public:
    //构造函数
    SM_Manager    (IX_Manager &ixm_, 
    		    RM_Manager &rmm_);
    //析构函数
    ~SM_Manager   ();                            

    //创建数据库
    RC CreateDb(const char *dbName);
    //销毁数据库
    RC DropDb(const char *dbName);

    //打开数据库
    RC OpenDb(const char *dbName);
    //关闭数据库
    RC CloseDb();

    //创建数据表
    RC CreateTable(const char *relName,
                   int attrCount,
                   AttrInfo *attributes);
    //销毁数据表
    RC DropTable(const char *relName);
    
    
    //创建索引
    RC CreateIndex(const char *relName,
                   const char *attrName);
    //销毁索引
    RC DropIndex(const char *relName,
                 const char *attrName);

   //加载
    RC Load(const char *relName,
            const char *fileName);
    
    //打印数据库的关系
    RC Help();
    //打印模式
    RC Help(const char *relName);
    //打印内容
    RC Print(const char *relName);

    //将指定参数设置为给定值
    RC Set(const char *paramName,
           const char *value);

    //检索与关系项关联的数据
    RC GetRelEntry(const char *relName, RelCatEntry &relEntry);
    //检索与特定属性关联的数据
    RC GetAttrEntry(const char *relName, const char *attrName, AttrCatEntry &attrEntry);
    //检索DataAttr的数据
    RC GetDataAttrInfo(const char *relName, int &attrCount, std::vector<DataAttrInfo> &attributes, bool sort = false);
    //更新relcat中指定关系表信息
    RC UpdateRelEntry(const char *relName, const RelCatEntry &relEntry);
    //更新attrcat中指定关系表指定属性信息
    RC UpdateAttrEntry(const char *relName, const char *attrName, const AttrCatEntry &attrEntry);
    //检索与特定属性关联的数据
private:
    //检索与关系项关联的数据
    RC GetRelCatEntry(const char *relName, RM_Record &rec);
    //检索与特定属性关联的数据
    RC GetAttrCatEntry(const char *relName, const char *attrName, RM_Record &rec);
};


//打印错误功能
void SM_PrintError(RC rc);


#define SM_REL_EXISTS            (START_SM_WARN + 0) //无效的RID
#define SM_REL_NOTEXIST          (START_SM_WARN + 1)
#define SM_ATTR_NOTEXIST         (START_SM_WARN + 2)
#define SM_INDEX_EXISTS          (START_SM_WARN + 3)
#define SM_INDEX_NOTEXIST        (START_SM_WARN + 4)
#define SM_FILE_FORMAT_INCORRECT (START_SM_WARN + 5)
#define SM_FILE_NOT_FOUND        (START_SM_WARN + 6)
#define SM_LASTWARN SM_FILE_NOT_FOUND


#define SM_CHDIR_FAILED    (START_SM_ERR - 0)
#define SM_CATALOG_CORRUPT (START_SM_ERR - 1)
#define SM_LASTERROR SM_CATALOG_CORRUPT

#endif // SM_H
