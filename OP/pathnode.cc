//
// Created by elias on 23-4-30.
//

#include "c.h"
#include "cost.h"
#include "list.h"
#include "paths.h"
#include "bitmapset.h"
#include "indexpath.h"

typedef enum
{
    COSTS_EQUAL,
    COSTS_BETTER1,
    COSTS_BETTER2,
    COSTS_DIFFERENT
} PathCostComparison;

#define STD_FUZZ_FACTOR 1.01

static PathCostComparison compare_path_costs_fuzzily(Path *path1, Path *path2, double fuzz_factor);

void
set_cheapest(RelOptInfo *parent_rel)
{
    Path       *cheapest_startup_path;
    Path       *cheapest_total_path;
    Path       *best_param_path;
    List       *parameterized_paths;
    ListCell   *p;

    cheapest_startup_path = cheapest_total_path = best_param_path = NULL;
    parameterized_paths = NIL;

    foreach(p, parent_rel->pathlist)
    {
        Path   *path = (Path *) lfirst(p);
        int     cmp;

        if (path->param_info)
        {
            parameterized_paths = lappend(parameterized_paths, path);

            if (cheapest_total_path)
            {
                continue;
            }

            if (best_param_path == NULL)
            {
                best_param_path = path;
            }
            else
            {
                switch (bms_subset_compare(PATH_REQ_OUTER(path),
                                           PATH_REQ_OUTER(best_param_path)))
                {
                    case BMS_EQUAL:
                        if (compare_path_costs(path, best_param_path, TOTAL_COST) < 0)
                        {
                            best_param_path = path;
                        }
                        break;
                    case BMS_SUBSET1:
                        best_param_path = path;
                        break;
                    case BMS_SUBSET2:
                        break;
                    case BMS_DIFFERENT:
                        break;

                }
            }
        }
        else
        {
            if (cheapest_total_path == NULL)
            {
                cheapest_startup_path = cheapest_total_path = path;
                continue;
            }
            cmp = compare_path_costs(cheapest_startup_path, path, STARTUP_COST);
            if (cmp > 0 ||
                (cmp == 0 &&
                 compare_pathkeys(cheapest_startup_path->pathkeys,
                                  path->pathkeys) == PATHKEYS_BETTER2))
            {
                cheapest_startup_path = path;
            }
            cmp = compare_path_costs(cheapest_total_path, path, TOTAL_COST);
            if (cmp > 0 ||
                (cmp == 0 &&
                 compare_pathkeys(cheapest_total_path->pathkeys, path->pathkeys) == PATHKEYS_BETTER2))
            {
                cheapest_total_path = path;
            }
        }
    }

    if (cheapest_total_path)
    {
        parameterized_paths = lcons(cheapest_total_path, parameterized_paths);
    }

    if (cheapest_total_path == NULL)
    {
        cheapest_total_path = best_param_path;
    }

    parent_rel->cheapest_startup_path = cheapest_startup_path;
    parent_rel->cheapest_total_path = cheapest_total_path;
    parent_rel->cheapest_parameterized_paths = parameterized_paths;
}

void
add_path(RelOptInfo *parent_rel, Path *new_path)
{
    bool        accept_new = true;
    int         insert_at = 0;
    List       *new_path_pathkeys;
    ListCell   *p1;

    new_path_pathkeys = new_path->param_info ? NIL : new_path->pathkeys;

    foreach(p1, parent_rel->pathlist)
    {
        Path   *old_path = (Path *) lfirst(p1);
        bool    remove_old = false;
        PathCostComparison  costcmp;
        PathKeysComparison  keyscmp;
        BMS_Comparison outercmp;

        costcmp = compare_path_costs_fuzzily(new_path, old_path, STD_FUZZ_FACTOR);

        if (costcmp != COSTS_DIFFERENT)
        {
            List   *old_path_pathkeys;
            old_path_pathkeys = old_path->param_info ? NIL : old_path->pathkeys;

            keyscmp = compare_pathkeys(new_path_pathkeys,
                                       old_path_pathkeys);

            if (keyscmp != PATHKEYS_DIFFERENT)
            {
                switch (costcmp)
                {
                    case COSTS_EQUAL:
                        outercmp = bms_subset_compare(PATH_REQ_OUTER(new_path),
                                                      PATH_REQ_OUTER(old_path));
                        if (keyscmp == PATHKEYS_BETTER1)
                        {
                            if ((outercmp == BMS_EQUAL ||
                                 outercmp == BMS_SUBSET1) &&
                                new_path->rows <= old_path->rows)
                            {
                                remove_old = true;
                            }
                        }
                        else if (keyscmp == PATHKEYS_BETTER2)
                        {
                            if ((outercmp == BMS_EQUAL ||
                                 outercmp == BMS_SUBSET2) &&
                                 new_path->rows >= old_path->rows)
                            {
                                accept_new = false;
                            }
                        }
                        else
                        {
                            if (outercmp == BMS_EQUAL)
                            {
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
                            }
                            else if (outercmp == BMS_SUBSET1 &&
                                     new_path->rows <= old_path->rows)
                            {
                                remove_old = true;
                            }
                            else if (outercmp == BMS_SUBSET2 &&
                                     new_path->rows >= old_path->rows)
                            {
                                accept_new = false;
                            }
                        }
                        break;
                    case COSTS_BETTER1:
                        if (keyscmp != PATHKEYS_BETTER2)
                        {
                            outercmp = bms_subset_compare(PATH_REQ_OUTER(new_path),
                                                          PATH_REQ_OUTER(old_path));
                            if ((outercmp == BMS_EQUAL ||
                                 outercmp == BMS_SUBSET1) &&
                                new_path->rows <= old_path->rows)
                            {
                                remove_old = true;
                            }
                        }
                        break;
                    case COSTS_BETTER2:
                        if (keyscmp != PATHKEYS_BETTER1)
                        {
                            outercmp = bms_subset_compare(PATH_REQ_OUTER(new_path),
                                                          PATH_REQ_OUTER(old_path));
                            if ((outercmp == BMS_EQUAL ||
                                 outercmp == BMS_SUBSET2) &&
                                new_path->rows >= old_path->rows)
                            {
                                accept_new = false;
                            }
                        }
                        break;
                    case COSTS_DIFFERENT:
                        break;
                }
            }
        }

        if (remove_old)
        {
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
create_index_path(PlannerInfo *root, IndexOptInfo *index, List *indexclauses, List *pathkeys, Relids required_outer, double loop_count)
{
    IndexPath  *pathnode = makeNode(IndexPath);
    RelOptInfo *rel = index->rel;

    pathnode->path.pathtype = T_IndexScan;
    pathnode->path.parent = rel;
    // TODO get_baserel_parampathinfo
//    pathnode->path.param_info = get_baserel_parampathinfo(root, rel, required_outer);

    pathnode->path.pathkeys = pathkeys;

    pathnode->indexinfo = index;
    pathnode->indexclauses = indexclauses;

    cost_index(pathnode, root, loop_count);

    return pathnode;
}

static PathCostComparison
compare_path_costs_fuzzily(Path *path1, Path *path2, double fuzz_factor)
{
#define CONSIDER_PATH_STARTUP_COST(p) \
    ((p)->param_info == NULL ? (p)->parent->consider_startup : (p)->parent->consider_param_startup)

    if (path1->total_cost > path2->total_cost * fuzz_factor)
    {
        if (CONSIDER_PATH_STARTUP_COST(path1) &&
            path2->startup_cost > path1->startup_cost * fuzz_factor)
        {
            return COSTS_DIFFERENT;
        }
        return COSTS_BETTER2;
    }

    if (path2->total_cost > path1->total_cost * fuzz_factor) {
        if (CONSIDER_PATH_STARTUP_COST(path2) &&
            path1->startup_cost > path2->startup_cost * fuzz_factor)
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
