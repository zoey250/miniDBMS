//
// Created by elias on 23-4-30.
//

#include "../utils/c.h"
#include "cost.h"
#include "../utils/list.h"

typedef enum
{
    COSTS_EQUAL,
    COSTS_BETTER1,
    COSTS_BETTER2,
    COSTS_DIFFERENT
} PathCostComparison;

#define STD_FUZZ_FACTOR 1.01

static PathCostComparison compare_path_costs_fuzzily(Path *path1,
                                                     Path *path2,
                                                     double fuzz_factor);

void
set_cheapest(RelOptInfo *parent_rel)
{
    Path       *cheapest_startup_path;
    Path       *cheapest_total_path;
    Path       *best_param_path;
    ListCell   *p;

    cheapest_startup_path = cheapest_total_path = best_param_path = NULL;

    foreach(p, parent_rel->pathlist)
    {
        Path   *path = (Path *) lfirst(p);
        int     cmp;

        if (path->pathtype == T_IndexScan)
        {
            IndexPath  *iPath = (IndexPath *) path;
            if (iPath->complex_condition_idx != -1)
            {
                continue;
            }
        }

        if (cheapest_total_path == NULL)
        {
            cheapest_startup_path = cheapest_total_path = path;
            continue;
        }
        cmp = compare_path_costs(cheapest_startup_path,
                                 path,
                                 STARTUP_COST);
        if (cmp >= 0)
        {
            cheapest_startup_path = path;
        }
        cmp = compare_path_costs(cheapest_total_path,
                                 path,
                                 TOTAL_COST);
        if (cmp >= 0)
        {
            cheapest_total_path = path;
        }
    }

    if (cheapest_total_path == NULL)
    {
        cheapest_total_path = best_param_path;
    }

    parent_rel->cheapest_startup_path = cheapest_startup_path;
    parent_rel->cheapest_total_path = cheapest_total_path;
}

void
add_path(RelOptInfo *parent_rel, Path *new_path)
{
    bool        accept_new = true;
    int         insert_at = 0;
    ListCell   *p1;


    // index use this
    foreach(p1, parent_rel->pathlist)
    {
        Path   *old_path = (Path *) lfirst(p1);
        bool    remove_old = false;
        PathCostComparison  costcmp;

        costcmp = compare_path_costs_fuzzily(new_path, old_path, STD_FUZZ_FACTOR);

        if (costcmp != COSTS_DIFFERENT)
        {
            switch (costcmp)
            {
                case COSTS_EQUAL:
                    if (new_path->rows < old_path->rows)
                    {
                        remove_old = true;
                    }
                    else if (new_path->rows > old_path->rows)
                    {
                        accept_new = false;
                    }
                    else if (compare_path_costs_fuzzily(new_path,
                                                        old_path,
                                                        1.0000000001))
                    {
                        remove_old = true;
                    }
                    else
                    {
                        accept_new = false;
                    }
                    break;
                case COSTS_BETTER1:
                    if (new_path->rows <= old_path->rows)
                    {
                        remove_old = true;
                    }
                    break;
                case COSTS_BETTER2:
                    if (new_path->rows >= old_path->rows)
                    {
                        accept_new = false;
                    }
                    break;
                case COSTS_DIFFERENT:
                    break;
            }
        }

        if (remove_old)
        {
            parent_rel->pathlist = foreach_delete_current(parent_rel->pathlist , p1);
            if (!IsA(old_path, IndexPath))
            {
                free(old_path);
            }
        }
        else
        {
            if (new_path->total_cost >= old_path->total_cost)
            {
                insert_at = foreach_current_index(p1) + 1;
            }
        }

        if (!accept_new)
        {
            break;
        }
    }

    if (accept_new)
    {
        parent_rel->pathlist = list_insert_nth(parent_rel->pathlist, insert_at, new_path);
    }
    else
    {
        if (!IsA(new_path, IndexPath))
        {
            free(new_path);
        }
    }
}

Path *
create_seqscan_path(RelOptInfo *rel)
{
    Path   *pathnode = makeNode(Path);

    pathnode->pathtype = T_SeqScan;
    pathnode->parent = rel;

    cost_seqscan(pathnode, rel);

    return pathnode;
}

int
compare_path_costs(Path *path1, Path *path2, CostSelect criterion)
{
    if (criterion == STARTUP_COST)
    {
        if (path1->startup_cost < path2->startup_cost)
        {
            return -1;
        }
        if (path1->startup_cost > path2->startup_cost)
        {
            return +1;
        }

        if (path1->total_cost < path2->total_cost)
        {
            return -1;
        }
        if (path1->total_cost > path2->total_cost)
        {
            return +1;
        }
    }
    else
    {
        if (path1->total_cost < path2->total_cost)
        {
            return -1;
        }
        if (path1->total_cost > path2->total_cost)
        {
            return +1;
        }

        if (path1->startup_cost < path2->startup_cost)
        {
            return -1;
        }
        if (path1->startup_cost > path2->startup_cost)
        {
            return +1;
        }
    }
    return 0;
}

IndexPath *
create_index_path(PlannerInfo *root, IndexOptInfo *index,
                  double loop_count)
{
    IndexPath  *pathnode = makeNode(IndexPath);
    RelOptInfo *rel = index->rel;

    pathnode->path.pathtype = T_IndexScan;
    pathnode->path.parent = rel;
    pathnode->path.pathtarget = rel->reltarget;

    pathnode->indexinfo = index;

    cost_index(pathnode, root, loop_count);

    return pathnode;
}

IndexPath *
create_complex_index_path(PlannerInfo *root, IndexOptInfo *index,
                          double loop_count, QL_Condition condition)
{
    IndexPath *pathnode = makeNode(IndexPath);
    RelOptInfo *rel = index->rel;

    pathnode->path.pathtype = T_IndexScan;
    pathnode->path.parent = rel;
    pathnode->path.pathtarget = rel->reltarget;

    pathnode->indexinfo = index;

    cost_complex_index(pathnode, root, loop_count, condition);

    return pathnode;
}

static PathCostComparison
compare_path_costs_fuzzily(Path *path1, Path *path2, double fuzz_factor)
{
    if (path1->total_cost > path2->total_cost * fuzz_factor)
    {
        if (path2->startup_cost > path1->startup_cost * fuzz_factor)
        {
            return COSTS_DIFFERENT;
        }
        return COSTS_BETTER2;
    }

    if (path2->total_cost > path1->total_cost * fuzz_factor) {
        if (path1->startup_cost > path2->startup_cost * fuzz_factor)
        {
            return COSTS_DIFFERENT;
        }
        return COSTS_BETTER1;
    }

    if (path1->startup_cost > path2->startup_cost * fuzz_factor)
    {
        return COSTS_BETTER2;
    }

    if (path2->startup_cost > path1->startup_cost * fuzz_factor)
    {
        return COSTS_BETTER1;
    }

    return COSTS_EQUAL;
}
