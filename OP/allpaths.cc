//
// Created by elias on 23-4-30.
//
#include "c.h"
#include "pathnodes.h"

static void set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);
static void set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel);

void
set_base_rel_pathlists(PlannerInfo *root)
{
    Index   rti;
    for (rti = 1; rti < root->simple_rel_array_size; rti++)
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
