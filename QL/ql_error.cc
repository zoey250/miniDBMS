//
// File:        ql_error.cc
// Description: QL_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ql.h"

using namespace std;

//
// 错误表
//
const char *QL_WarnMsg[] = {
    "number of values to insert does not match",
    "value type does not match",
    "string value too long",
    "attribute does not exist",
    "attribute types does not match",
    "ambiguous attribute name",
    "operation forbidden",
    "attribute should not be null",
    "a record with the same primary key already exits",
};

const char *QL_ErrorMsg[] = {

};


/**
 *
 * 打印错误信息，参数rc为错误码，函数根据错误码输出错误信息
 */
void QL_PrintError(RC rc) {
    // Check the return code is within proper limits
    if (rc >= START_QL_WARN && rc <= QL_LASTWARN)
        // Print warning
        cerr << "QL warning: " << QL_WarnMsg[rc - START_QL_WARN] << "\n";
    // Error codes are negative, so invert everything
    else if ((-rc >= -START_QL_ERR) && -rc <= -QL_LASTERROR) {
        // Print error
        cerr << "QL error: " << QL_ErrorMsg[-rc + START_QL_ERR] << "\n";
    } else if (rc == 0)
        cerr << "QL_PrintError called with return code of 0\n";
    else {
        // Print error
        cerr << "rc was " << rc << endl;
        cerr << "START_QL_ERR was " << START_QL_ERR << endl;
        cerr << "QL_LASTERROR was " << QL_LASTERROR << endl;
        cerr << "QL error: " << rc << " is out of bounds\n";
    }
}
