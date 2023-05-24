//
// Created by elias on 23-5-24.
//

#include "cost.h"

NestPath *
create_nestloop_path(Path *outer_path, Path *inner_path)
{
    NestPath   *pathnode = makeNode(NestPath);

    pathnode->jpath.path.pathtype = T_NestLoop;
    pathnode->jpath.outerjoinpath = outer_path;
    pathnode->jpath.innerjoinpath = inner_path;

    cost_nestloop(pathnode);

    return pathnode;
}
