#ifndef MICRODBMS_SM_MANAGER_H
#define MICRODBMS_SM_MANAGER_H

#include "../SM/sm.h"

#include <algorithm>
#include <memory>
#include <cassert>
#include <stddef.h>
#include <unistd.h>
#include "sys/stat.h"
#include <dirent.h>

//长度
static const int kCwdLen = 256;


#pragma mark - 构造函数和析构函数
//构造函数
SM_Manager::SM_Manager(IX_Manager &ixm_, RM_Manager &rmm_) {
    this->ixm = &ixm_;
    this->rmm = &rmm_;
}

//析构函数
SM_Manager::~SM_Manager() {}


#pragma mark - 数据库操作库
/**
1. 创建数据库
输入参数：数据库名称
*/
RC SM_Manager::CreateDb(const char *dbName) {
    RC rc = OK_RC;
    //判断是否已经存在，是目录,return OK

    ///home/quanjunyuan/CLionProjects/rebaseDB/cmake-build-debug/
    std::string temp = std::string("/home/quanjunyuan/CLionProjects/rebaseDB/cmake-build-debug/") + std::string(dbName);
    const char *file_path = temp.c_str();
    //printf("path : %s \n", file_path);
    DIR *dir = opendir(file_path);
    if(NULL != dir) {
        printf("There is a folder with the same name in the current directory. Please try again after deleting or changing the database name. \n");
        closedir(dir);
        return rc;
    }
    //create database
    int status = mkdir(dbName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //创建成功
    if (status == 0) {
        printf("Successfully created database %s. \n",dbName);
    }
    else {
        printf("Failed to create database %s. \n",dbName);
        rc = 1;
    }

    if (chdir(dbName) < 0) {
        //无法进入数据库目录
        printf("chdir error to %s \n", dbName);
        rc = 1;
    }
    else {
        //成功进入后，创建文件relcat和attrcat
        // Calculate the size of entries in relcat:
        int relCatRecSize = sizeof(RelCatEntry);
        int attrCatRecSize = sizeof(AttrCatEntry);

        if ((rc = rmm->CreateFile("relcat", relCatRecSize))) {
            printf("Trouble creating relcat. Exiting \n",dbName);
            exit(1);
        }

        if ((rc = rmm->CreateFile("attrcat", attrCatRecSize))) {
            printf("Trouble creating attrcat. Exiting \n",dbName);
            exit(1);
        }

        // Adding relation metadata of relcat and attrcat
        const char *(relName[MAXNAME + 1]) = {"relcat", "attrcat"};
        const char *(attrName[MAXNAME + 1]) = {
                "relName", "tupleLength", "attrCount", "indexCount", "recordCount",
                "relName", "attrName", "offset", "attrType", "attrSize", "attrDisplayLength", "attrSpecs", "indexNo"
        };

        RM_FileHandle handle;
        RID rid;
        rmm->OpenFile("relcat", handle);
        RelCatEntry relEntry;

        memcpy(relEntry.relName, relName[0], MAXNAME + 1);
        relEntry.tupleLength = sizeof(RelCatEntry);
        relEntry.attrCount = 5;
        relEntry.indexCount = 0;
        handle.InsertRec((const char *)&relEntry, rid);

        memcpy(relEntry.relName, relName[1], MAXNAME + 1);
        relEntry.tupleLength = sizeof(AttrCatEntry);
        relEntry.attrCount = 8;
        relEntry.indexCount = 0;
        handle.InsertRec((const char *)&relEntry, rid);

        rmm->CloseFile(handle);

        // Adding attribute metadata of relcat and attrcat
        rmm->OpenFile("attrcat", handle);
        AttrCatEntry attrEntry;
        attrEntry.attrSpecs = ATTR_SPEC_NOTNULL;
        attrEntry.indexNo = -1;

        memcpy(attrEntry.relName, relName[0], MAXNAME + 1);
        memcpy(attrEntry.attrName, attrName[0], MAXNAME + 1);
        attrEntry.offset = offsetof(RelCatEntry, relName);
        attrEntry.attrType = STRING;
        attrEntry.attrDisplayLength = MAXNAME + 1;
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[1], MAXNAME + 1);
        attrEntry.offset = offsetof(RelCatEntry, tupleLength);
        attrEntry.attrType = INT;
        attrEntry.attrDisplayLength = sizeof(int);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[2], MAXNAME + 1);
        attrEntry.offset = offsetof(RelCatEntry, attrCount);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[3], MAXNAME + 1);
        attrEntry.offset = offsetof(RelCatEntry, indexCount);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[4], MAXNAME + 1);
        attrEntry.offset = offsetof(RelCatEntry, recordCount);
        handle.InsertRec((const char *)&attrEntry, rid);

        memcpy(attrEntry.relName, relName[1], MAXNAME + 1);
        memcpy(attrEntry.attrName, attrName[5], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, relName);
        attrEntry.attrType = STRING;
        attrEntry.attrDisplayLength = MAXNAME + 1;
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[6], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, attrName);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[7], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, offset);
        attrEntry.attrType = INT;
        attrEntry.attrDisplayLength = sizeof(int);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[8], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, attrType);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[9], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, attrSize);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[10], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, attrDisplayLength);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[11], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, attrSpecs);
        handle.InsertRec((const char *)&attrEntry, rid);
        memcpy(attrEntry.attrName, attrName[12], MAXNAME + 1);
        attrEntry.offset = offsetof(AttrCatEntry, indexNo);
        handle.InsertRec((const char *)&attrEntry, rid);

        rmm->CloseFile(handle);
    }

    if (chdir("..") < 0) {
        //无法进入数据库目录
        printf("Unable to return to the previous directory. \n");
        rc = 1;
    }
    return rc;
}

/**
2. 销毁数据库
输入参数：数据库名称
*/
//销毁数据库
RC SM_Manager::DropDb(const char *dbName) {
    //closeDB
    this->CloseDb();
    //库名
    std::string file_path = dbName;

    struct stat st;
    if(lstat(file_path.c_str(),&st) == -1) {
        printf("The specified database %s has incorrect information. \n",dbName);
        return -1;
    }

    //是常规文件
    if(S_ISREG(st.st_mode)) {
        if(unlink(file_path.c_str()) == -1) {
            printf("Failed to delete %s file. \n",dbName);
            return -1;
        }
    }
        //是目录
    else if(S_ISDIR(st.st_mode)) {
        if(dbName == "." || dbName == "..") {
            printf("Database %s name is wrong. \n",dbName);
            return -1;
        }

        //遍历删除指定数据库下的文件
        DIR* dirp = opendir(file_path.c_str());
        if(!dirp) {
            return -1;
        }
        struct dirent *dir;
        struct stat st;
        //遍历
        while((dir = readdir(dirp)) != NULL) {
            if(strcmp(dir->d_name,".") == 0
               || strcmp(dir->d_name,"..") == 0)
            {
                continue;
            }
            std::string sub_path = file_path + '/' + dir->d_name;
            if(lstat(sub_path.c_str(),&st) == -1) {
                continue;
            }
            // 如果是普通文件，则unlink
            if(S_ISREG(st.st_mode)) {
                unlink(sub_path.c_str());
            }
            else {
                printf("The database %s contains illegal files and cannot be deleted. \n",dbName);
                continue;
            }
        }
        //删除目录（需要空）
        if(rmdir(file_path.c_str()) == -1) {
            closedir(dirp);
            printf("Failed to delete database %s. \n",dbName);
            return -1;
        }
        closedir(dirp);
    }
    printf("Successfully deleted database %s. \n",dbName);
    return 0;
}

/**
 3. 打开对应于数据库的文件夹并进入该文件夹
 输入参数：数据库名称
 */
RC SM_Manager::OpenDb(const char *dbName) {
    //检查库名是否有效
    if (chdir(dbName) != 0) return SM_CHDIR_FAILED;
    //打开并保存relcat文件句柄
    TRY(rmm->OpenFile("relcat", relcat));
    //打开并保存attrcat文件句柄
    TRY(rmm->OpenFile("attrcat", attrcat));
    return 0;
}

/**
 4. 关闭数据库，因此关闭任何打开文件
 输入参数：无
*/
RC SM_Manager::CloseDb() {
    //关闭relcat文件
    TRY(rmm->CloseFile(relcat));
    //关闭attrcat文件
    TRY(rmm->CloseFile(attrcat));
    //返回上一级目录
    if (chdir("..") != 0) return SM_CHDIR_FAILED;
    return 0;
}

#pragma mark - 数据库操作 表
/**
 1.创建数据库表
 输入参数：relName表名，attrCount属性数量，attributes属性
 */
RC SM_Manager::CreateTable(const char *relName, int attrCount, AttrInfo *attributes) {
    RM_FileScan scan;
    RM_Record rec;
    
    //检查relcat文件 验证是否已经包含改表
    TRY(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(RelCatEntry, relName),
                      EQ_OP, (void *)relName));
    if (scan.GetNextRec(rec) != RM_EOF) return SM_REL_EXISTS;
    TRY(scan.CloseScan());

    //在relcat和attrcat中插入
    RID rid;
    int indexNo = 0;
    short offset = 0;
    std::vector<short> nullableOffsets;
    //对每个属性遍历
    for (int i = 0; i < attrCount; ++i) {
        //初始化一个属性目录项
        AttrCatEntry attrEntry;
        memset(&attrEntry, 0, sizeof attrEntry);
        strcpy(attrEntry.relName, relName);
        strcpy(attrEntry.attrName, attributes[i].attrName);
        attrEntry.offset = offset;
        attrEntry.attrType = attributes[i].attrType;
        // + 1 为了终结符 '\0'
        attrEntry.attrSize = attributes[i].attrType == STRING ? attributes[i].attrLength + 1 : 4;
        attrEntry.attrDisplayLength = attributes[i].attrLength;
        attrEntry.attrSpecs = attributes[i].attrSpecs;
        if (!(attrEntry.attrSpecs & ATTR_SPEC_NOTNULL))
            nullableOffsets.push_back(offset);
        offset += upper_align<4>(attrEntry.attrSize);
        //是主键
        if (attrEntry.attrSpecs & ATTR_SPEC_PRIMARYKEY) {
            attrEntry.indexNo = indexNo++;
        } else {
            attrEntry.indexNo = -1;
        }
        //向attrcat文件中插入记录（对应每一个属性）
        TRY(attrcat.InsertRec((const char *)&attrEntry, rid));
    }
    
    //新建表
    RelCatEntry relEntry;
    memset(&relEntry, 0, sizeof(RelCatEntry));
    strcpy(relEntry.relName, relName);
    relEntry.tupleLength = offset;
    relEntry.attrCount = attrCount;
    relEntry.indexCount = 0;
    relEntry.recordCount = 0;
    
    TRY(rmm->CreateFile(relName, relEntry.tupleLength,
                        (short)nullableOffsets.size(), &nullableOffsets[0]));
    //对主键建立索引
    for (int i = 0; i < attrCount; ++i)
        if (attributes[i].attrSpecs & ATTR_SPEC_PRIMARYKEY)
            TRY(ixm->CreateIndex(relName, relEntry.indexCount++, attributes[i].attrType, attributes[i].attrLength));
    //向relcat文件中插入记录（对应新建的表）
    TRY(relcat.InsertRec((const char *)&relEntry, rid));

    //强制写回磁盘，确保atttcat和relcat的更改
    TRY(relcat.ForcePages());
    TRY(attrcat.ForcePages());

    return 0;
}

/**
 2.删除数据库表，并删除其所有索引，删除relcat和attrcat内的相关信息
 输入参数：relName表名
*/
RC SM_Manager::DropTable(const char *relName) {
    AttrCatEntry *attrEntry;
    RM_FileScan scan;
    RM_Record rec;
    RID rid;
    //删除该表
    TRY(rmm->DestroyFile(relName));
    
    //检索attrcat文件
    TRY(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(AttrCatEntry, relName),
                      EQ_OP, (void *)relName));
    RC retcode;
    //没达到文件末尾
    while ((retcode = scan.GetNextRec(rec)) != RM_EOF) {
        if (retcode) return retcode;
        TRY(rec.GetData((char *&)attrEntry));
        //对于建立过索引的属性要对应删除建立的索引文件
        if (attrEntry->indexNo != -1)
            TRY(ixm->DestroyIndex(relName, attrEntry->indexNo));
        TRY(rec.GetRid(rid));
        //删除属于目标表的属性项
        TRY(attrcat.DeleteRec(rid));
    }
    //关闭检索器
    TRY(scan.CloseScan());

    //检索relcat文件
    TRY(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(RelCatEntry, relName),
                      EQ_OP, (void *)relName));
    TRY(scan.GetNextRec(rec));
    TRY(rec.GetRid(rid));
    //删除目标表项
    TRY(relcat.DeleteRec(rid));
    TRY(scan.CloseScan());

    //强制写回磁盘，确保atttcat和relcat的更改
    TRY(relcat.ForcePages());
    TRY(attrcat.ForcePages());

    return 0;
}


#pragma mark - 数据库操作 索引
/**
 1. 创建索引
 输入参数：relName关系名称，attrName属性名称
 */
RC SM_Manager::CreateIndex(const char *relName, const char *attrName) {
    RM_Record relRec, attrRec;
    RelCatEntry *relEntry;
    AttrCatEntry *attrEntry;

    //首先验证当前指定属性上是否存在索引，只有不存在时才可以新建新的索引
    TRY(GetRelCatEntry(relName, relRec));
    TRY(relRec.GetData((char *&)relEntry));
    TRY(GetAttrCatEntry(relName, attrName, attrRec));
    TRY(attrRec.GetData((char *&)attrEntry));
    if (attrEntry->indexNo != -1) return SM_INDEX_EXISTS;

    //获取索引标号
    int indexNo = relEntry->indexCount;
    IX_IndexHandle indexHandle;
    RM_FileHandle fileHandle;
    RM_FileScan scan;
    RM_Record rec;
    //调用创建索引函数
    TRY(ixm->CreateIndex(relName, indexNo, attrEntry->attrType, attrEntry->attrSize));
    //打开主文件和索引文件
    TRY(rmm->OpenFile(relName, fileHandle));
    TRY(ixm->OpenIndex(relName, indexNo, indexHandle));
    TRY(scan.OpenScan(fileHandle, INT, sizeof(int), 0, NO_OP, NULL));
    RC retcode;
    //遍历 直到已经到主文件的结尾
    while ((retcode = scan.GetNextRec(rec)) != RM_EOF) {
        if (retcode) return retcode;
        RID rid;
        char *data;
        TRY(rec.GetRid(rid));
        TRY(rec.GetData(data));
        //向索引文件中插入项
        TRY(indexHandle.InsertEntry(data + attrEntry->offset, rid));
    }
    //关闭主文件和索引文件
    TRY(scan.CloseScan());
    TRY(ixm->CloseIndex(indexHandle));
    TRY(rmm->CloseFile(fileHandle));

    //更新relcat attrcat文件相应的属性值
    attrEntry->indexNo = indexNo;
    ++relEntry->indexCount;
    TRY(relcat.UpdateRec(relRec));
    TRY(attrcat.UpdateRec(attrRec));

    //强制写回磁盘，确保atttcat和relcat的更改
    TRY(relcat.ForcePages());
    TRY(attrcat.ForcePages());

    return 0;
}

static std::string filename_gen(const char* fileName, int indexNo) {
    std::ostringstream oss;
    oss << fileName << "." << indexNo;
    return oss.str();
}

/**
 2. 销毁索引
 输入参数：relName关系名称，attrName属性名称
*/
RC SM_Manager::DropIndex(const char *relName, const char *attrName) {
    RM_Record relRec, attrRec;
    RelCatEntry *relEntry;
    AttrCatEntry *attrEntry;
    
    //获取relcat和attrcat中对应的项，验证当前属性上是否存在索引
    TRY(GetRelCatEntry(relName, relRec));
    TRY(relRec.GetData((char *&)relEntry));
    TRY(GetAttrCatEntry(relName, attrName, attrRec));
    TRY(attrRec.GetData((char *&)attrEntry));
    
    if (attrEntry->indexNo == -1) return SM_INDEX_NOTEXIST;

    //保存当前删除的索引号
    int curDeleteIndexNo = attrEntry->indexNo;

    //删除索引
    TRY(ixm->DestroyIndex(relName, attrEntry->indexNo));
    attrEntry->indexNo = -1;
    --relEntry->indexCount;
    
    //更新relcat和attrcat文件
    TRY(relcat.UpdateRec(relRec));
    TRY(attrcat.UpdateRec(attrRec));

    //遍历表内其他属性 如果还有建立索引的要确认索引号是否大于attrEntry->indexNo
    //如果其他属性索引号大于attrEntry->indexNo，则需要减1，并修改对应的索引文件名
    //relName表的属性数量
    int attrCount = relEntry->attrCount;
    RM_FileScan scan;
    //开始扫描attrcat文件
    TRY(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(AttrCatEntry, relName),
                      EQ_OP, (void *)relName));
    RC retcode;
    //如果文件没有到末尾
    while ((retcode = scan.GetNextRec(attrRec)) != RM_EOF) {
        if (retcode) return retcode;
        TRY(attrRec.GetData((char *&) attrEntry));

        //非空的属性值个数
        if (attrEntry->indexNo > curDeleteIndexNo) {
            //修改对应索引文件名称

            std::string oldIndexFileName = filename_gen(relName, attrEntry->indexNo);
            --attrEntry->indexNo;
            std::string newIndexFileName = filename_gen(relName, attrEntry->indexNo);
            //printf(" old: %s\n new: %s", oldIndexFileName.c_str(), newIndexFileName.c_str());
            int result = rename(oldIndexFileName.c_str(), newIndexFileName.c_str());
            //printf("result : %d\n", result);
        }
        TRY(attrcat.UpdateRec(attrRec));
    }

    //强制写回磁盘，确保atttcat和relcat的更改
    TRY(relcat.ForcePages());
    TRY(attrcat.ForcePages());

    return 0;
}

#pragma mark - 数据库操作 目录
/**
 1.获取关系项信息
 输入参数：relName关系名称
*/
RC SM_Manager::GetRelEntry(const char *relName, RelCatEntry &relEntry) {
    RM_Record rec;
    char *data;
    //获取relcat指定项信息
    TRY(GetRelCatEntry(relName, rec));
    TRY(rec.GetData(data));
    //保存目录项信息
    relEntry = *(RelCatEntry *)data;

    return 0;
}

/**
 2.获取属性信息
 输入参数：relName关系名称
         attrName属性名
         attrEntry 属性目录实体项
*/
RC SM_Manager::GetAttrEntry(const char *relName, const char *attrName, AttrCatEntry &attrEntry) {
    RM_Record rec;
    char *data;

    //获取attrcat指定项信息
    TRY(GetAttrCatEntry(relName, attrName, rec));
    TRY(rec.GetData(data));
    //保存目录项信息
    attrEntry = *(AttrCatEntry *)data;

    return 0;
}

/**
 3.获取relcat指定项信息
  输入参数：relName关系名称
          rec RM_Record实体
 */
RC SM_Manager::GetRelCatEntry(const char *relName, RM_Record &rec) {
    RM_FileScan scan;
    //开始扫描relcat文件
    TRY(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(RelCatEntry, relName),
                      EQ_OP, (void *)relName));
    //获取该条记录
    RC retcode = scan.GetNextRec(rec);
    //获取失败
    if (retcode == RM_EOF) return SM_REL_NOTEXIST;
    if (retcode) return retcode;
    //关闭扫描器
    TRY(scan.CloseScan());

    return 0;
}

/**
 4.获取attrcat指定项信息
 输入参数：relName关系名称
         attrName属性名
         rec RM_Record实体
*/
RC SM_Manager::GetAttrCatEntry(const char *relName, const char *attrName, RM_Record &rec) {
    AttrCatEntry *attrEntry;
    RM_FileScan scan;
    RID rid;

    //开始扫描attrcat文件
    TRY(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(AttrCatEntry, relName),
                      EQ_OP, (void *)relName));
    RC retcode;
    bool found = false;
    //如果没有到文件尾
    while ((retcode = scan.GetNextRec(rec)) != RM_EOF) {
        if (retcode) return retcode;
        TRY(rec.GetData((char *&)attrEntry));
        //如果找到
        if (!strncmp(attrEntry->attrName, attrName, MAXNAME)) {
            found = true;
            break;
        }
    }
    //关闭扫描器
    TRY(scan.CloseScan());
    //如果没找到返回相应的提示
    if (!found) return SM_ATTR_NOTEXIST;

    return 0;
}

/**
 5.更新relcat文件指定项信息
  输入参数：relName关系名称
          relEntry relcat文件实体项
 */
RC SM_Manager::UpdateRelEntry(const char *relName, const RelCatEntry &relEntry) {
    RM_Record rec;
    char *data;
    //获取relcat指定项
    TRY(GetRelCatEntry(relName, rec));
    TRY(rec.GetData(data));
    //更新数据
    *(RelCatEntry *)data = relEntry;
    TRY(relcat.UpdateRec(rec));
    //强制写回磁盘
    TRY(relcat.ForcePages());

    return 0;
}


/**
 6.更新attrcat文件的指定项信息
 输入参数：relName关系名称
         attrName属性名
         attrEntry attrcat文件实体项
*/
RC SM_Manager::UpdateAttrEntry(const char *relName, const char *attrName, const AttrCatEntry &attrEntry) {
    RM_Record rec;
    char *data;
    //获取attrcat指定项
    TRY(GetAttrCatEntry(relName, attrName, rec));
    TRY(rec.GetData(data));
    //更新数据
    *(AttrCatEntry *)data = attrEntry;
    TRY(attrcat.UpdateRec(rec));
    //强制写回磁盘
    TRY(attrcat.ForcePages());

    return 0;
}

/**
 7.获取指定数据表的属性
 输入参数：relName关系名称
         attrCount属性数量
         attributes保存属性值信息的向量
         sort是否需要排序的标识位
*/
RC SM_Manager::GetDataAttrInfo(const char *relName, int &attrCount, std::vector<DataAttrInfo> &attributes, bool sort) {
    RM_FileScan scan;
    RM_Record rec;
    RID rid;
    RelCatEntry relEntry;
    AttrCatEntry *attrEntry;

    //获取relca文件中对应于relName表的项
    TRY(GetRelEntry(relName, relEntry));
    //relName表的属性数量
    attrCount = relEntry.attrCount;
    attributes.clear();
    attributes.resize((unsigned long)attrCount);
    //开始扫描attrcat文件
    TRY(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(AttrCatEntry, relName),
                      EQ_OP, (void *)relName));
    RC retcode;
    int i = 0, nullableIndex = 0;
    //如果文件没有到末尾
    while ((retcode = scan.GetNextRec(rec)) != RM_EOF) {
        if (retcode) return retcode;
        //获取属性信息
        TRY(rec.GetData((char *&)attrEntry));
        strcpy(attributes[i].relName, attrEntry->relName);
        strcpy(attributes[i].attrName, attrEntry->attrName);
        attributes[i].offset = attrEntry->offset;
        attributes[i].attrType = attrEntry->attrType;
        attributes[i].attrSpecs = attrEntry->attrSpecs;
        attributes[i].attrSize = attrEntry->attrSize;
        attributes[i].attrDisplayLength = attrEntry->attrDisplayLength;
        attributes[i].indexNo = attrEntry->indexNo;
        //非空的属性值个数
        if ((attrEntry->attrSpecs & ATTR_SPEC_NOTNULL) == 0) {
            attributes[i].nullableIndex = nullableIndex++;
        } else {
            attributes[i].nullableIndex = -1;
        }
//        VLOG(2) << attributes[i].attrName << " " << attributes[i].attrSpecs << " " << attributes[i].nullableIndex;
        ++i;
    }
    TRY(scan.CloseScan());
    if (i != attrCount) return SM_CATALOG_CORRUPT;

    //排序
    if (sort) {
        std::sort(attributes.begin(), attributes.end(),
                  [](const DataAttrInfo &a, const DataAttrInfo &b) { return a.offset < b.offset; });
    }

    return 0;
}


#pragma mark - 数据库操作 加载
/**
 1.将内容从指定文件加载到指定关系中
 输入参数：relName给定关系名，fileName给定文件名
*/
RC SM_Manager::Load(const char *relName, const char *fileName) {
    RelCatEntry relEntry;
    //检索与relcat中relName相关的记录和数据
    TRY(GetRelEntry(relName, relEntry));
    int attrCount;
    std::vector<DataAttrInfo> attributes;
    
    //获取relName的属性信息
    TRY(GetDataAttrInfo(relName, attrCount, attributes, true));

    RM_FileHandle fileHandle;
    RID rid;
    ARR_PTR(data, char, relEntry.tupleLength);
    ARR_PTR(isnull, bool, relEntry.attrCount);
    ARR_PTR(indexHandles, IX_IndexHandle, attrCount);
    //索引
    for (int i = 0; i < attrCount; ++i)
        if (attributes[i].indexNo != -1)
            TRY(ixm->OpenIndex(relName, attributes[i].indexNo, indexHandles[i]));
    TRY(rmm->OpenFile(relName, fileHandle));
    FILE *file = fopen(fileName, "r");
    if (!file) return SM_FILE_NOT_FOUND;

    char buffer[MAXATTRS * MAXSTRINGLEN + 1];
    int cnt = 0;
    while (!feof(file)) {
        fscanf(file, "%[^\n]\n", buffer);
        memset(data, 0, (size_t)relEntry.tupleLength);
        int p = 0, q = 0, l = (int)strlen(buffer);
        for (int i = 0; i < attrCount; ++i) {
            for (q = p; q < l && buffer[q] != ','; ++q);
            if (q == l && i != attrCount - 1) {
                std::cerr << cnt + 1 << ":" << q << " " << "too few columns" << std::endl;
                return SM_FILE_FORMAT_INCORRECT;
            }
            buffer[q] = 0;
            if (p == q) {
                if (attributes[i].attrSpecs & ATTR_SPEC_NOTNULL) {
                    std::cerr << cnt + 1 << ":" << p << " " << "null value for non-nullable attribute" << std::endl;
                    return SM_FILE_FORMAT_INCORRECT;
                }
                isnull[attributes[i].nullableIndex] = true;
            } else {
                if (!(attributes[i].attrSpecs & ATTR_SPEC_NOTNULL))
                    isnull[attributes[i].nullableIndex] = false;
                switch (attributes[i].attrType) {
                    //int
                    case INT: {
                        char *end = NULL;
                        *(int *)(data + attributes[i].offset) = strtol(buffer + p, &end, 10);
                        if (end == data + attributes[i].offset) {
                            std::cerr << cnt + 1 << ":" << q << " " << "incorrect integer" << std::endl;
                            return SM_FILE_FORMAT_INCORRECT;
                        }
                        break;
                    }
                    //float
                    case FLOAT: {
                        char *end = NULL;
                        *(float *)(data + attributes[i].offset) = strtof(buffer + p, &end);
                        if (end == data + attributes[i].offset) {
                            std::cerr << cnt + 1 << ":" << q << " " << "incorrect float" << std::endl;
                            return SM_FILE_FORMAT_INCORRECT;
                        }
                        break;
                    }
                    //string
                    case STRING: {
                        if (q - p > attributes[i].attrDisplayLength) {
                            std::cerr << cnt + 1 << ":" << q << " " << "string too long" << std::endl;
                            return SM_FILE_FORMAT_INCORRECT;
                        }
                        strcpy(data + attributes[i].offset, buffer + p);
                        break;
                    }
                }
            }
            p = q + 1;
        }
        // LOG(INFO) << "=================================== " << cnt;
        TRY(fileHandle.InsertRec(data, rid, isnull));
        for (int i = 0; i < attrCount; ++i) {
            if (attributes[i].indexNo != -1) {
                TRY(indexHandles[i].InsertEntry(data + attributes[i].offset, rid));
                // TRY(indexHandles[i].Traverse());
            }
        }
        ++cnt;
    }
    VLOG(2) << "file loaded";

    relEntry.recordCount = cnt;
    TRY(UpdateRelEntry(relName, relEntry));

    for (int i = 0; i < attrCount; ++i)
        if (attributes[i].indexNo != -1) {
            // TRY(indexHandles[i].Traverse());
            TRY(ixm->CloseIndex(indexHandles[i]));
        }
    TRY(rmm->CloseFile(fileHandle));

    std::cout << cnt << " values loaded." << std::endl;

    return 0;
}



#pragma mark - 数据库操作 打印
/**
 1.打印关系中的所有元组
 输入参数：relName关系名称
*/
RC SM_Manager::Print(const char *relName) {
    int attrCount;
    std::vector<DataAttrInfo> attributes;
    //获取指定表的属性信息
    TRY(GetDataAttrInfo(relName, attrCount, attributes, true));

    //打印
    Printer printer(attributes);
    //打印头部信息
    printer.PrintHeader(std::cout);

    RM_FileHandle fileHandle;
    RM_FileScan scan;
    RM_Record rec;
    //打开指定表文件，打开扫描器
    TRY(rmm->OpenFile(relName, fileHandle));
    TRY(scan.OpenScan(fileHandle, INT, sizeof(int), 0, NO_OP, NULL));
    RC retcode;
    //如果没到文件尾
    while ((retcode = scan.GetNextRec(rec)) != RM_EOF) {
        if (retcode) return retcode;
        char *data;
        bool *isnull;
        TRY(rec.GetData(data));
        TRY(rec.GetIsnull(isnull));
        //打印数据
        printer.Print(std::cout, data, isnull);
    }
    //关闭文件和扫描器
    TRY(scan.CloseScan());
    TRY(rmm->CloseFile(fileHandle));

    //打印尾部信息
    printer.PrintFooter(std::cout);

    return 0;
}

/**
 2.打印有关指定关系中所有属性的信息, 打印的信息包括：relName,attributeName,偏移,属性类型,属性长度,索引号
 输入参数：relName给定关系名
*/
RC SM_Manager::Help(const char *relName) {
    int attrCount;
    std::vector<DataAttrInfo> attributes;
    //打印attrcat文件信息
    TRY(GetDataAttrInfo("attrcat", attrCount, attributes));
    Printer printer(attributes);
    //打印relName对应的关系表信息
    TRY(GetDataAttrInfo(relName, attrCount, attributes, true));

    printer.PrintHeader(std::cout);
    for (int i = 0; i < attrCount; ++i)
        printer.Print(std::cout, (char *)&attributes[i], NULL);

    printer.PrintFooter(std::cout);

    return 0;
}

RC SM_Manager::Set(const char *paramName, const char *value) {
    return 0;
}

RC SM_Manager::Help() {
    return 0;
}


#endif //MICRODBMS_SM_MANAGER_H
