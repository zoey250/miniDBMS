//
// Created by elias on 23-5-24.
//

#ifndef MICRODBMS_JOINPATH_H
#define MICRODBMS_JOINPATH_H

extern inline bool is_leaf(Path *path);
extern inline bool is_join(Path *path);
extern NestPath *create_nestloop_path(Path *outer_path, Path *inner_path);

#endif //MICRODBMS_JOINPATH_H
