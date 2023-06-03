//
// Created by elias on 6/1/23.
//

#ifndef MICRODBMS_STATISTICS_H
#define MICRODBMS_STATISTICS_H

#include "sm.h"
#include "ql_internal.h"

static char *REL_STATISTICS = "relstatistics";
static char *REL_NAME = "relname";
static char *ATTR_NAME = "attrname";
static char *BUCKET_NUM = "bucketnum";
static char *VALUE = "value";

class SS_Manager
{
public:
    SS_Manager(QL_Manager &qlm, SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~SS_Manager();

    RC AnalyzeTable(const char *relName);

private:
    QL_Manager *pQlm;
    SM_Manager *pSmm;   // SM_Manager对象
    IX_Manager *pIxm;   // IX_Manager对象
    RM_Manager *pRmm;   // RM_Manager对象

    void create_rel_statistics();
    RC sample(const char *relname);
    RC insert_or_update(DataAttrInfo &attrInfo, std::vector<std::vector<int>> &buckets);
    static void build_values(DataAttrInfo &attrInfo, std::vector<std::vector<int>> &buckets, Value *values);
    static RC reservoir_sampling(const char *relname,int reservoir_size, char **reservoir, int tuple_length);
    RC executor_sample(int attrCount, AttrList attrInfo, int reservoir_size, char **reservoir);
    RC check_rel(const char *name);
};

#endif //MICRODBMS_STATISTICS_H
