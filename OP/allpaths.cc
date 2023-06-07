//
// Created by elias on 23-4-30.
//
#include <math.h>
#include "../utils/c.h"
#include "pathnodes.h"
#include "parser.h"
#include "printer.h"
#include "cost.h"
#include "paths.h"

static void set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);
static void set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);
static List* init_index_list(RelOptInfo *rel,
                             std::vector<std::pair<IX_IndexHandle, IndexOptInfo>> indexVector,
                             std::vector<QL_Condition> simpleConditions,
                             std::vector<QL_Condition> complexConditions);
static void set_base_rel_sizes(PlannerInfo *root);
static void set_base_rel_pathlists(PlannerInfo *root);

PlannerInfo *
init_planner_info(AttrList finalProjections,
                  std::vector<AttrList> simpleProjections,
                  int nRelations, const char *const *relations,
                  std::vector<RM_FileHandle> fileHandles,
                  std::vector<RelCatEntry> relEntries,
                  std::vector<std::vector<std::pair<IX_IndexHandle, IndexOptInfo>>> indexVector,
                  std::vector<std::vector<QL_Condition>> simpleConditions,
                  std::vector<QL_Condition> complexConditions,
                  std::vector<AttrList> attrInfo)
{
    PlannerInfo    *root = new PlannerInfo();
    root->type = T_PlannerInfo;
    root->simple_rel_array_size = nRelations;
    root->simple_rel_array = (RelOptInfo **) malloc(sizeof(RelOptInfo *) * (nRelations + 1));
    for (int i = 0; i < nRelations; ++i)
    {
        RelOptInfo *rel = new RelOptInfo;
        Cardinality tuples = relEntries[i].recordCount;
        short       recordPerPage = fileHandles[i].getRecordPerPage();
        BlockNumber pages = 0;
        Index       relid = i;
        Relids      relids = NULL;

        pages = (tuples + recordPerPage - 1) / recordPerPage;
        relids = bms_add_member(relids, relid);

        for (auto condition : complexConditions)
        {
            if (strcmp(relations[i], condition.lhsAttr.relName) == 0 ||
                strcmp(relations[i], condition.rhsAttr.relName) == 0)
            {
                rel->complexconditions.push_back(condition);
            }
        }

        rel->type = T_RelOptInfo;
        rel->relids = relids;
        rel->name = relations[i];
        rel->pages = pages;
        rel->tuples = relEntries[i].recordCount;
        rel->baserestrictcost = {.startup = 0, .per_tuple = 0};
        rel->relid = relid;

        rel->indexlist = init_index_list(rel,
                                         indexVector[i],
                                         simpleConditions[i],
                                         rel->complexconditions);
        rel->conditions = simpleConditions[i];
        rel->reltarget = simpleProjections[i];

        rel->pathlist = NULL;
        rel->attrinfo = attrInfo[i];
        root->simple_rel_array[i] = rel;
    }
    root->finalProjections = finalProjections;
    return root;
}

void
cost_estimate(PlannerInfo *root)
{
    set_base_rel_sizes(root);
    set_base_rel_pathlists(root);
}

static void
set_base_rel_sizes(PlannerInfo *root)
{
    Index   rti;
    for (rti = 0; rti < root->simple_rel_array_size; rti++)
    {
        RelOptInfo *rel = root->simple_rel_array[rti];
        set_baserel_size_estimates(rel);
    }

}

static void
set_base_rel_pathlists(PlannerInfo *root)
{
    Index   rti;
    for (rti = 0; rti < root->simple_rel_array_size; rti++)
    {
        RelOptInfo *rel = root->simple_rel_array[rti];
        if (rel == NULL)
        {
            continue;
        }
        set_rel_pathlist(root, rel);
    }
}

static void
set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel)
{
    set_plain_rel_pathlist(root, rel);

    set_cheapest(rel);
}

static void
set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel)
{
    add_path(rel, create_seqscan_path(rel));

    create_index_paths(root, rel);
}

static List*
init_index_list(RelOptInfo *rel,
                std::vector<std::pair<IX_IndexHandle, IndexOptInfo>> indexVector,
                std::vector<QL_Condition> simpleConditions,
                std::vector<QL_Condition> complexConditions)
{
    List   *list = NIL;
    for (int i = 0; i < indexVector.size(); ++i)
    {
        IX_IndexHandle  indexHandle = indexVector[i].first;
        IndexOptInfo    oldInfo = indexVector[i].second;
        IndexOptInfo   *indexInfo = new IndexOptInfo;

        for (int j = 0; j < simpleConditions.size(); ++j)
        {
            if (strcmp(oldInfo.attrName, simpleConditions[j].lhsAttr.attrName) == 0)
            {
                if (strcmp(simpleConditions[j].lhsAttr.relName, oldInfo.relName) == 0)
                {
                    indexInfo->conditions.push_back(simpleConditions[j]);
                }
            }
        }

        for (int j = 0; j < complexConditions.size(); ++j)
        {
            if ((strcmp(oldInfo.attrName, complexConditions[j].lhsAttr.attrName) == 0 &&
                 strcmp(oldInfo.relName, complexConditions[j].lhsAttr.relName) == 0) ||
                (strcmp(oldInfo.attrName, complexConditions[j].rhsAttr.attrName) == 0 &&
                 strcmp(oldInfo.relName, complexConditions[j].rhsAttr.relName) == 0))
            {
                indexInfo->complexconditions.push_back(complexConditions[j]);
            }
        }

        indexInfo->type = T_IndexOptInfo;
        indexInfo->rel = rel;

        int     bucketnum = (int) rel->tuples / 2;
        int     leafnum = bucketnum / indexHandle.getB();
        int     treeheight = ceil(log(leafnum)/ log(indexHandle.getB()));
        /*
        int     pages = 1;
        int     pages_per_layer = 1;
        for (int j = 1; j < treeheight; ++j)
        {
             pages_per_layer *= indexHandle.getB();
             pages += pages_per_layer;
        }
        indexInfo->pages = pages;
         */
        indexInfo->pages = leafnum;
        indexInfo->tuples = rel->tuples;
        indexInfo->tree_height = treeheight;
        indexInfo->indexkey = oldInfo.indexkey;
        indexInfo->unique = oldInfo.unique;
        list = lappend(list, indexInfo);
    }
    return list;
}