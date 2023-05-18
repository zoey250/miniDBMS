//
// Created by elias on 23-4-30.
//
#include "../utils/c.h"
#include "pathnodes.h"
#include "parser.h"
#include "printer.h"
#include "cost.h"
#include "paths.h"

static void set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);
static void set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);
static List* init_index_list(RelOptInfo *rel, AttrList attrInfo);
static void set_base_rel_sizes(PlannerInfo *root);
static void set_base_rel_pathlists(PlannerInfo *root);

PlannerInfo *
init_planner_info(int nSelAttrs, const RelAttr *selAttrs,
                  int nRelations, const char *const *relations,
                  const std::vector<RM_FileHandle> fileHandles,
                  std::vector<RelCatEntry> relEntries,
                  std::vector<AttrList> attrInfo,
                  int nConditions, const Condition *conditions)
{
    PlannerInfo    *root = (PlannerInfo *)malloc(sizeof(PlannerInfo));
    root->type = T_PlannerInfo;
    root->simple_rel_array_size = nRelations;
    root->simple_rel_array = (RelOptInfo **) malloc(sizeof(RelOptInfo *) * (nRelations + 1));
    for (int i = 0; i < nRelations; ++i)
    {
        RelOptInfo *rel = (RelOptInfo *) malloc(sizeof(RelOptInfo));
        Cardinality tuples = relEntries[i].recordCount;
        short       recordPerPage = fileHandles[i].getRecordPerPage();
        BlockNumber pages = 0;
        Index       relid = i;
        Relids      relids = NULL;

        pages = (tuples + recordPerPage - 1) / recordPerPage;
        relids = bms_add_member(relids, relid);

        rel->type = T_RelOptInfo;
        rel->relids = relids;
        rel->name = relations[i];
        rel->pages = pages;
        rel->tuples = relEntries[i].recordCount;
        rel->baserestrictcost = {.startup = 0, .per_tuple = 0};
        rel->relid = relid;

        rel->indexlist = init_index_list(rel, attrInfo[i]);
        // TODO reltarget and baserestrictinfo

        root->simple_rel_array[i] = rel;
    }
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
init_index_list(RelOptInfo *rel, AttrList attrInfo)
{
    /*
     * TODO finish index init
    for (int i = 0; i < attrInfo.size(); ++i)
    {
        if (attrInfo[i].indexNo != -1)
        {
            IndexOptInfo   *indexInfo = (IndexOptInfo *) malloc(sizeof(IndexOptInfo));

            indexInfo->type = T_IndexOptInfo;
            indexInfo->rel = rel;
        }
    }
    */
    return NIL;
}