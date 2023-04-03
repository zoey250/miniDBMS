#include "ix.h"
#include "ix_internal.h"

#include <string>
#include <sstream>
#include <stddef.h>

//生成索引文件名称
static std::string filename_gen(const char* fileName, int indexNo) {
    std::ostringstream oss;
    oss << fileName << "." << indexNo;
    return oss.str();
}

#pragma mark - 1.构造函数&析构函数
/**
 构造函数
*/
IX_Manager::IX_Manager(PF_Manager &pfm) {
    this->pfm = &pfm;
}

/**
 析构函数
*/
IX_Manager::~IX_Manager() { }


#pragma mark - 2.创建/销毁/打开/关闭索引文件
/**
 此方法为文件 fileName 创建标号为 indexNo 的索引，该索引可以保存在文件 fileName.indexNo 里
 被索引属性的类型和长度分别在参数 attributeType 和 attributeSize 中指明
 参数fileName为文件名
 参数indexNo为索引标号
 参数attrType为被索引属性的类型
 参数attrLength为被索引属性的长度
 */
RC IX_Manager::CreateIndex(const char *fileName,
                           int indexNo,
                           AttrType attrType,
                           int attrLength) {
    //检查属性长度
    int attrSize = upper_align<4>(attrLength);
    if (offsetof(IX_PageHeader, entries) + attrSize + 2 * offsetof(Entry, key) > PF_PAGE_SIZE) {
        return IX_ATTR_TOO_LARGE;
    }
    //生成索引文件名称
    std::string indexFileName = filename_gen(fileName, indexNo);
    //创建索引文件
    TRY(pfm->CreateFile(indexFileName.c_str()));
    PF_FileHandle fileHandle;
    PF_PageHandle pageHandle;
    //打开索引文件
    TRY(pfm->OpenFile(indexFileName.c_str(), fileHandle));

    //创建索引文件header
    IX_FileHeader *fileHeader;
    TRY(fileHandle.AllocatePage(pageHandle));
    TRY(pageHandle.GetData(CVOID(fileHeader)));
    fileHeader->attrType = attrType;
    fileHeader->attrLength = attrLength;
    fileHeader->root = 1;
    fileHeader->firstFreePage = kLastFreePage;
    //标记被修改，强制写回磁盘
    TRY(fileHandle.MarkDirty(0));
    TRY(fileHandle.UnpinPage(0));

    //创建根结点
    IX_PageHeader *root;
    TRY(fileHandle.AllocatePage(pageHandle));
    TRY(pageHandle.GetData(CVOID(root)));
    root->type = kLeafNode;
    root->childrenNum = 0;
    Entry* root_first_entry = (Entry*)(root->entries);
    root_first_entry->pageNum = kNullNode;
    //标记被修改，强制写回磁盘
    TRY(fileHandle.MarkDirty(1));
    TRY(fileHandle.UnpinPage(1));

    TRY(pfm->CloseFile(fileHandle));

    // RM_Manager rmm(*pfm);
    // TRY(rmm.CreateFile((indexFileName + "r").c_str(), sizeof(RID)));
    return 0;
}

/**
 该方法通过删除页式文件系统中的索引文件来删除 fileName 上标号为 indexNo 的索引
 参数fileName为文件名
 参数indexNo为索引号
*/
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    //获取文件名
    std::string indexFileName = filename_gen(fileName, indexNo);
    //销毁文件为索引名的文件
    TRY(pfm->DestroyFile(indexFileName.c_str()));

    // TRY(rmm.DestroyFile((indexFileName + "r").c_str()));
    return 0;
}

/**
 该方法通过打开页式文件系统中的索引文件来打开 fileName 上标号为 indexNo 的索引。
 如果成功执行，indexHandle 对象应指代该打开的索引
 参数fileName为文件名
 参数indexNo为索引标号
 参数indexHandle为指代该打开的索引的IX_IndexHandle
 (可通过创建多个的 indexHandle 来多次打开同一个索引，但同一时刻只有一个 indexHandle 能修改该索引)
*/
RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
    //获取索引名
    std::string indexFileName = filename_gen(fileName, indexNo);
    PF_FileHandle &fileHandle = indexHandle.pfHandle;
    PF_PageHandle pageHandle;
    IX_FileHeader *fileHeader;

    //打开文件
    TRY(pfm->OpenFile(indexFileName.c_str(), fileHandle));
    //获取首页
    TRY(fileHandle.GetFirstPage(pageHandle));
    TRY(pageHandle.GetData(CVOID(fileHeader)));
    //设置indexHandle属性
    indexHandle.attrType = fileHeader->attrType;
    indexHandle.attrLength = fileHeader->attrLength;
    indexHandle.root = fileHeader->root;
    indexHandle.firstFreePage = fileHeader->firstFreePage;
    indexHandle.isHeaderDirty = false;
    //取消固定第一页
    TRY(fileHandle.UnpinPage(0));
    // the initialization MUST come after information in the
    // header copied into the handle
    indexHandle.__initialize();

    // RM_FileHandle &rmHandle = indexHandle.rmHandle;
    // TRY(rmm.OpenFile((indexFileName + "r").c_str(), rmHandle));

    return 0;
}

/**
 此方法通过关闭页式文件系统中的索引文件来关闭 indexHandle 对象指代的索引
 参数indexHandle为指定的IX_IndexHandle
*/
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    PF_FileHandle &fileHandle = indexHandle.pfHandle;

    //如果文件被修改过 则需要更新
    if (indexHandle.isHeaderDirty) {
        PF_PageHandle pageHandle;
        IX_FileHeader *fileHeader;

        TRY(fileHandle.GetFirstPage(pageHandle));
        TRY(pageHandle.GetData(CVOID(fileHeader)));
        fileHeader->root = indexHandle.root;
        fileHeader->firstFreePage = indexHandle.firstFreePage;
        
        //标记被修改并取消固定
        TRY(fileHandle.MarkDirty(0));
        TRY(fileHandle.UnpinPage(0));
    }

    //未被修改 直接关闭
    TRY(pfm->CloseFile(fileHandle));

    // RM_FileHandle &rmHandle = indexHandle.rmHandle;
    // TRY(rmm.CloseFile(rmHandle));
    return 0;
}
