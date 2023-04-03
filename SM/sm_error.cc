#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm.h"

using namespace std;

//错误表
static const char *SM_WarnMsg[] = {
        "relation already exists",
        "relation does not exist",
        "attribute does not exist for given relation",
        "index already exists for given attribute",
        "index does not exist for given attribute",
        "file to load has incorrect format",
        "file not found",
        "length of string-typed attribute should not exceed MAXSTRINGLEN=255"
};

//错误信息
static const char *SM_ErrorMsg[] = {
        "chdir command execution failed",
        "database catalog file is corrupt",
};

//向cerr发送与RM返回码相对应的消息，输入为所需消息的返回码
void SM_PrintError(RC rc) {
    //检查返回码是否在适当的范围内
    if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
        cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
    //错误代码为负，因此请反转所有内容
    else if ((-rc >= -START_SM_ERR) && -rc <= -SM_LASTERROR) {
        cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
    } else if (rc == 0)
        cerr << "SM_PrintError called with return code of 0\n";
    else {
        cerr << "rc was " << rc << endl;
        cerr << "START_SM_ERR was " << START_SM_ERR << endl;
        cerr << "SM_LASTERROR was " << SM_LASTERROR << endl;
        cerr << "SM error: " << rc << " is out of bounds\n";
    }
}
