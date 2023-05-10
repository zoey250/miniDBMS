//
// Created by elias on 23-4-30.
//

#ifndef MICRODBMS_PATHNODES_H
#define MICRODBMS_PATHNODES_H
#include "block.h"
#include "attnum.h"
#include "nodes.h"
#include "list.h"
#include "bitmapset.h"
#include "primnodes.h"

typedef Bitmapset *Relids;
typedef struct Expr Expr;

typedef struct PlannerInfo PlannerInfo;

struct PlannerInfo
{
    NodeTag     type;

    int         simple_rel_array_size;
    struct RelOptInfo **simple_rel_array;
    Cardinality total_table_pages;
    List       *eq_classes;
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

    Cardinality     rows;

    // TODO ??
    bool            consider_startup;
    bool            consider_param_startup;

    // TODO __input__
    List           *indexlist;

    BlockNumber     pages;
    Cardinality     tuples;

    Bitmapset      *eclass_indexes;
    QualCost        baserestrictcost;
    Index           relid;
    bool            has_eclass_joins;

    // TODO __output__
    List           *pathlist;

    struct Path    *cheapest_startup_path;
    struct Path    *cheapest_total_path;
    List           *cheapest_parameterized_paths;

} RelOptInfo;

typedef struct ParamPathInfo
{
    NodeTag     type;

    Relids      ppi_req_outer;
    Cardinality ppi_rows;
    List       *ppi_clauses;
} ParamPathInfo;

typedef struct Path
{
    NodeTag     type;
    NodeTag     pathtype;

    RelOptInfo *parent;
    Cardinality rows;
    Cost        startup_cost;
    Cost        total_cost;

    ParamPathInfo  *param_info;

    List       *pathkeys;
} Path;

#define PATH_REQ_OUTER(path) \
    ((path)->param_info ? (path)->param_info->ppi_req_outer : (Relids) NULL)

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
    BlockNumber pages;
    Cardinality tuples;

    int         tree_height;

    // TODO single column, maybe don't need array.
    int         nkeycolumns;
    int        *indexkeys;

    List       *indrestrictinfo;

    bool        unique;
};

typedef struct IndexClause
{
    NodeTag     type;
    struct RestrictInfo *rinfo;
    List       *indexquals;
    AttrNumber  indexcol;
} IndexClause;

typedef struct EquivalenceClass
{
    NodeTag     type;
    List       *ec_members;
    List       *ec_sources;
    List       *ec_derives;
} EquivalenceClass;

typedef struct EquivalenceMember
{
    NodeTag     type;

    Expr       *em_expr;
    Relids      em_relids;
} EquivalenceMember;

typedef struct RestrictInfo
{
    NodeTag type;

    Expr   *clause;

    bool    can_join;

    Relids  clause_relids;
    Relids  required_relids;
//    Relids  outer_relids;
    Relids  left_relids;
    Relids  right_relids;

    EquivalenceClass   *parent_ec;

    EquivalenceMember  *left_em;
    EquivalenceMember  *right_em;

} RestrictInfo;

typedef struct IndexPath
{
    Path        path;
    IndexOptInfo   *indexinfo;
    List       *indexclauses;
    Cost        indextotalcost;
    Selectivity indexselectivity;
} IndexPath;

extern void set_cheapest(RelOptInfo *parent_rel);
extern void add_path(RelOptInfo *parent_rel, Path *new_path);
extern Path *create_seqscan_path(RelOptInfo *rel);
extern void create_index_paths(PlannerInfo *root, RelOptInfo *rel);
extern int compare_path_costs(Path *path1, Path *path2, CostSelect criterion);
extern IndexPath *create_index_path(PlannerInfo *root, IndexOptInfo *index,
                                    List *indexclauses, List *pathkeys,
                                    Relids required_outer, double loop_count);

#endif //MICRODBMS_PATHNODES_H
