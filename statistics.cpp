//
// Created by elias on 5/28/23.
//

#include <iostream>
#include <algorithm>
#include "ql.h"
#include "ql_iterator.h"
#include "statistics/statistics.h"

using namespace std;

PF_Manager  pfm;
RM_Manager  rmm(pfm);
IX_Manager  ixm(pfm);
SM_Manager  smm(ixm, rmm);
QL_Manager  qlm(smm, ixm, rmm);

static void create_rel_statistics();
static RC sample(const char *relname);
static RC insert_or_update(DataAttrInfo &attrInfo, vector<vector<int>> buckets);

int main()
{
    string databasename;

    cout << "input database: ";
    cin >> databasename;
    cout << endl;

    const char *current_db = databasename.c_str();

    int ret = smm.OpenDb(current_db);
    if (ret == SM_CHDIR_FAILED)
    {
        fprintf(stderr, "database %s does not exist.\n", current_db);
        return 0;
    }

    RM_FileScan scan;
    TRY(scan.OpenScan(smm.getRelcat(), STRING, MAXNAME + 1,
                      offsetof(RelCatEntry, relName),
                      EQ_OP, (void *) REL_STATISTICS));

    RM_Record   rec;
    ret = scan.GetNextRec(rec);
    scan.CloseScan();

    if (ret == RM_EOF)
    {
        create_rel_statistics();
    }

    while (true)
    {
        string  relname;
        cout << "input analyze relname: ";
        cin >> relname;
        cout << endl;

        if (relname == "exit")
        {
            break;
        }

        const char *current_rel = relname.c_str();

        TRY(scan.OpenScan(smm.getRelcat(), STRING, MAXNAME + 1,
                            offsetof(RelCatEntry, relName),
                            EQ_OP, (void *) current_rel));

        ret = scan.GetNextRec(rec);
        scan.CloseScan();
        if (ret == RM_EOF)
        {
            fprintf(stderr, "relname %s does not exist.\n", current_rel);
            continue;
        }

        sample(current_rel);

    }
    smm.CloseDb();
}

static void
create_rel_statistics()
{
    AttrInfo    attrInfos[4];
    attrInfos[0].attrName = REL_NAME;
    attrInfos[0].attrType = STRING;
    attrInfos[0].attrLength = MAXNAME + 1;
    attrInfos[0].attrSpecs = ATTR_SPEC_NOTNULL;

    attrInfos[1].attrName = ATTR_NAME;
    attrInfos[1].attrType = STRING;
    attrInfos[1].attrLength = MAXNAME + 1;
    attrInfos[1].attrSpecs = ATTR_SPEC_NOTNULL;

    attrInfos[2].attrName = BUCKET_NUM;
    attrInfos[2].attrType = INT;
    attrInfos[2].attrLength = 5;
    attrInfos[2].attrSpecs = ATTR_SPEC_NOTNULL;

    attrInfos[3].attrName = VALUE;
    attrInfos[3].attrType = STRING;
    attrInfos[3].attrLength = 4000;
    attrInfos[3].attrSpecs = ATTR_SPEC_NOTNULL;

    smm.CreateTable(REL_STATISTICS, 4, attrInfos);
}

static bool
cmp(int a, int b)
{
    return a < b;
}

static RC
sample(const char *relname)
{
    RelCatEntry relentry;
    int         attrCount;
    AttrList    attrInfo;
    TRY(smm.GetRelEntry(relname, relentry));
    TRY(smm.GetDataAttrInfo(relname, attrCount, attrInfo, true));
    int         numtuple = relentry.recordCount;

    QL_Iterator    *iter = new QL_FileScanIterator(relname);

    int         retcode;
    RM_Record   record;

    int         reservoir_size = numtuple / 100;
    char      **reservoir = (char **) malloc(sizeof(char*) * reservoir_size);

    int         idx = 0;

    while((retcode = iter->GetNextRec(record)) != RM_EOF)
    {
        char       *data;
        bool       *isnull;

        if (retcode)
        {
            return retcode;
        }

        TRY(record.GetData(data));
        TRY(record.GetIsnull(isnull));

        if (idx < reservoir_size)
        {
            char   *_data = (char *) malloc(sizeof(char) * relentry.tupleLength);
            memcpy(_data, data, sizeof(char) * relentry.tupleLength);
            reservoir[idx] = _data;
            idx++;
        }
        else
        {
            int d = rand() % (idx + 1);

            if (d < reservoir_size)
            {
                char   *_data = (char *) malloc(sizeof(char) * relentry.tupleLength);
                memcpy(_data, data, sizeof(char) * relentry.tupleLength);
                free(reservoir[d]);
                reservoir[d] = _data;
            }
        }
    }

    for (int i = 0; i < attrCount; ++i)
    {
        if (attrInfo[i].attrType == INT)
        {
            int values[reservoir_size];
            int bucket_depth = 20;
            int bucket_count = reservoir_size / bucket_depth;
            if (reservoir_size % bucket_depth != 0)
            {
                bucket_count++;
            }
            vector<vector<int>> buckets(bucket_count);

            for (int j = 0; j < reservoir_size; ++j)
            {
                int     value;
                char   *data = reservoir[j];
                memcpy(&value, (data + attrInfo[i].offset), sizeof(int));
                values[j] = value;
            }

            sort(values, values + reservoir_size, cmp);

            int     bucket_id = 0;
            if (reservoir_size <= bucket_depth)
            {
                for (int j = 0; j < reservoir_size; ++j)
                {
                    buckets[bucket_id].push_back(values[j]);
                }
            }
            else
            {
                for (int j = 0; j < bucket_depth; ++j)
                {
                    buckets[bucket_id].push_back(values[j]);
                }

                int     num = bucket_depth;

                for (int j = bucket_depth; j < reservoir_size; ++j)
                {
                    if (values[j] == values[j - 1])
                    {
                        buckets[bucket_id].push_back(values[j]);
                    }
                    else
                    {
                        if (num >= bucket_depth)
                        {
                            num = 0;
                            bucket_id++;
                        }
                        buckets[bucket_id].push_back(values[j]);
                        num++;
                    }
                }
            }

            insert_or_update(attrInfo[i], buckets);
        }
    }

    for (int i = 0; i < reservoir_size; ++i)
    {
        free(reservoir[i]);
    }
    free(reservoir);

    return 0;
}

static RC
insert_or_update(DataAttrInfo &attrInfo, vector<vector<int>> buckets)
{
    QL_Iterator    *iter = new QL_FileScanIterator(REL_STATISTICS);

    int             retcode;
    RM_Record       record;

    int             statisticsAttrCount;
    AttrList        statisticsAttrList;

    TRY(smm.GetDataAttrInfo(REL_STATISTICS, statisticsAttrCount, statisticsAttrList, true));

    while ((retcode = iter->GetNextRec(record)) != RM_EOF)
    {
        char   *data;
        bool   *isnull;

        if (retcode)
        {
            return retcode;
        }

        TRY(record.GetData(data));
        TRY(record.GetIsnull(isnull));

    }

    RelAttr rel_name =
            {
                .relName = REL_STATISTICS,
                .attrName = REL_NAME,
            };
    Value   rel_value =
            {
                .type = VT_STRING,
                .data = attrInfo.relName,
            };
    Condition   rel_condition =
            {
                .lhsAttr = rel_name,
                .op = EQ_OP,
                .bRhsIsAttr = false,
                .rhsValue = rel_value,
            };

    RelAttr attr_name =
            {
                .relName = REL_STATISTICS,
                .attrName = ATTR_NAME,
            };
    Value   attr_value =
            {
                    .type = VT_STRING,
                    .data = attrInfo.attrName,
            };
    Condition   attr_condition =
            {
                .lhsAttr = attr_name,
                .op = EQ_OP,
                .bRhsIsAttr = false,
                .rhsValue = attr_value,
            };

    Condition conditions[] = {rel_condition, attr_condition};
    qlm.Delete(REL_STATISTICS, 2, conditions);

    size_t bucket_size = buckets.size();

    string value_str = "";
    int begin = buckets[0].front();
    int end = buckets[0].back();
    value_str += "(" + to_string(begin) + "," + to_string(end) + ")";
    for (int i = 1; i < bucket_size; ++i)
    {
        if (buckets[i].empty())
        {
            bucket_size = i;
            break;
        }
        begin = buckets[i].front();
        end = buckets[i].back();
        value_str += ", (" + to_string(begin) + "," + to_string(end) + ")";
    }

    Value   bucket_num_value =
            {
                    .type = VT_INT,
                    .data = (void *)&bucket_size,
            };

    Value   value_value =
            {
                .type = VT_STRING,
                .data = (void *) value_str.c_str(),
            };

    Value values[] = {rel_value, attr_value, bucket_num_value, value_value};

    qlm.Insert(REL_STATISTICS, 4, values);
    return 0;
}
