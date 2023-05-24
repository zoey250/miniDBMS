//
// Created by elias on 23-4-30.
//

#ifndef MICRODBMS_PATHNODES_H
#define MICRODBMS_PATHNODES_H
#include "block.h"
#include "attnum.h"
#include "nodes.h"
#include "../utils/list.h"
#include "../utils/bitmapset.h"
#include "primnodes.h"
#include "parser.h"
#include "ql_internal.h"

typedef Bitmapset *Relids;
typedef struct Expr Expr;

typedef struct PlannerInfo PlannerInfo;

struct PlannerInfo
{
    NodeTag     type;

    int         simple_rel_array_size;
    struct RelOptInfo **simple_rel_array;

    AttrList    finalProjections;
};

typedef struct QualCost
{
    Cost    startup;
    Cost    per_tuple;
} QualCost;

typedef struct RelOptInfo
{
    NodeTag         type;
    Relids          relids;

    // __input__
    const char     *name;
    List           *indexlist;

    BlockNumber     pages;
    Cardinality     tuples;

//    List           *baserestrictinfo;
    std::vector<QL_Condition> conditions;
    QualCost        baserestrictcost;
    Index           relid;

    AttrList        reltarget;

    // __output__
    Cardinality     rows;
    List           *pathlist;

    struct Path    *cheapest_startup_path;
    struct Path    *cheapest_total_path;

} RelOptInfo;

typedef struct Path
{
    NodeTag     type;
    NodeTag     pathtype;

    RelOptInfo *parent;
    AttrList    pathtarget;
    Cardinality rows;
    Cost        startup_cost;
    Cost        total_cost;

} Path;

typedef struct PathKey
{
    NodeTag     type;
} PathKey;

typedef enum CostSelect
{
    STARTUP_COST, TOTAL_COST
} CostSelect;


typedef struct IndexOptInfo IndexOptInfo;

struct IndexOptInfo
{
    NodeTag     type;

    RelOptInfo *rel;

    char        relName[MAXNAME + 1];
    char        attrName[MAXNAME + 1];

    BlockNumber pages;
    Cardinality tuples;

    int         tree_height;

    int        indexkey;

    List       *indrestrictinfo;
    std::vector<QL_Condition> conditions;

    bool        unique;
    bool        flag = false;
};

typedef struct IndexClause
{
    NodeTag     type;
    struct RestrictInfo *rinfo;
    List       *indexquals;
} IndexClause;

typedef struct RestrictInfo
{
    NodeTag type;

    Expr   *clause;

    bool    can_join;

    Relids  clause_relids;
    Relids  required_relids;
    Relids  left_relids;
    Relids  right_relids;

} RestrictInfo;

typedef struct IndexPath
{
    Path        path;
    IndexOptInfo   *indexinfo;
    Cost        indextotalcost;
    Selectivity indexselectivity;
} IndexPath;

typedef struct JoinPath
{
    Path    path;
    Path   *outerjoinpath;
    Path   *innerjoinpath;

} JoinPath;

typedef struct NestPath
{
    JoinPath    jpath;
} NestPath;

typedef struct PathTarget
{
    NodeTag     type;
    QualCost    cost;
} PathTarget;

extern void set_cheapest(RelOptInfo *parent_rel);
extern void add_path(RelOptInfo *parent_rel, Path *new_path);
extern Path *create_seqscan_path(RelOptInfo *rel);
extern void create_index_paths(PlannerInfo *root, RelOptInfo *rel);
extern int compare_path_costs(Path *path1, Path *path2, CostSelect criterion);
extern IndexPath *create_index_path(PlannerInfo *root, IndexOptInfo *index,
                                    double loop_count);

#endif //MICRODBMS_PATHNODES_H
