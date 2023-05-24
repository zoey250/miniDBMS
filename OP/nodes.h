//
// Created by elias on 23-4-30.
//

#ifndef MICRODBMS_NODES_H
#define MICRODBMS_NODES_H

#include "stdlib.h"
#include "string.h"

typedef enum NodeTag {
    T_Invalid = 0,
    T_List = 1,
    T_IntList,
    T_OidList,
    T_SeqScan,
    T_Path,
    T_IndexPath,
    T_OpExpr,
    T_IndexClause,
    T_Var,
    T_Const,
    T_IndexScan,
    T_RestrictInfo,
    T_Bitmapset,
    T_PlannerInfo,
    T_RelOptInfo,
    T_IndexOptInfo,
    T_NestPath,
    T_NestLoop,
} NodeTag;

typedef double Selectivity;
typedef double Cost;
typedef double Cardinality;
//typedef long long int Cardinality;

typedef struct Node
{
    NodeTag     type;
} Node;

#define newNode(size, tag) \
({  Node   *_result; \
    _result = (Node *) malloc(size); \
    memset(_result, 0, size); \
    _result->type = (tag); \
    _result; \
})

#define nodeTag(nodeptr)        (((const Node*)(nodeptr))->type)

#define makeNode(_type_)        ((_type_ *) newNode(sizeof(_type_), T_##_type_))
#define IsA(nodeptr, _type_)    (nodeTag(nodeptr) == T_##_type_)

#define castNode(_type_, nodeptr) ((_type_ *) (nodeptr))

#endif //MICRODBMS_NODES_H
