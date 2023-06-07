//
// Created by elias on 23-5-24.
//

#include "cost.h"
#include "joinpath.h"

NestPath *
create_nestloop_path(Path *outer_path, Path *inner_path)
{
    NestPath   *pathnode = makeNode(NestPath);

    pathnode->jpath.path.pathtype = T_NestLoop;
    pathnode->jpath.outerjoinpath = outer_path;
    pathnode->jpath.innerjoinpath = inner_path;

    if (is_leaf(outer_path))
    {
        for (int i = 0; i < outer_path->parent->complexconditions.size(); ++i)
        {
            pathnode->jpath.outercomplexconditions.push_back(outer_path->parent->complexconditions[i]);
        }
    }
    else if (is_join(outer_path))
    {
        auto   *joinPath = (JoinPath *) outer_path;
        for (int i = 0; i < joinPath->outercomplexconditions.size(); ++i)
        {
            pathnode->jpath.outercomplexconditions.push_back(joinPath->outercomplexconditions[i]);
        }
    }

    if (is_leaf(inner_path))
    {
        for (int i = 0; i < inner_path->parent->complexconditions.size(); ++i)
        {
            pathnode->jpath.innercomplexconditions.push_back(inner_path->parent->complexconditions[i]);
        }
    }
    else if (is_join(inner_path))
    {
        auto   *joinPath = (JoinPath *) inner_path;
        for (int i = 0; i < joinPath->innercomplexconditions.size(); ++i)
        {
            pathnode->jpath.innercomplexconditions.push_back(joinPath->innercomplexconditions[i]);
        }
    }

    cost_nestloop(pathnode);

    return pathnode;
}

bool
is_leaf(Path *path)
{
    if (path->pathtype == T_SeqScan ||
        path->pathtype == T_IndexScan)
    {
        return true;
    }
    return false;
}

bool
is_join(Path *path)
{
    if (IsA(path, NestPath))
    {
        return true;
    }
    return false;
}