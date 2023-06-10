# 代价估计
## 接口
``` c
void cost_estimate(PlannerInfo *root)
```
代价估计的入参和出参都是`PlannerInfo`，它提供了代价估计所需要的信息，运行该函数后，会创建一些路径，并将每个路径的代价计算并保存，由之后的动态规划使用。  
`PlannerInfo`的结构如下  
```c
struct PlannerInfo
{
    NodeTag     type;
    int         simple_rel_array_size;
    struct RelOptInfo **simple_rel_array;
    AttrList    finalProjections;
};
```
其中`simple_rel_array`记录着每张表的信息，`finalProjections`记录着最终投影信息  
`RelOptInfo`的结构如下  
```c
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
    std::vector<QL_Condition> complexconditions;
    QualCost        baserestrictcost;
    Index           relid;

    AttrList        reltarget;

    // __output__
    Cardinality     rows;
    List           *pathlist;
    AttrList        attrinfo;

    struct Path    *cheapest_startup_path;
    struct Path    *cheapest_total_path;

} RelOptInfo;
```  
其中`__input__`中保存的就是需要的基本信息，`__output__`中保存了返回的路径等相关信息  
`Path`是路径，它是一个基类，还用`IndexPath`, `JoinPath`等派生的，根据功能丰富`Path`结构。`Path`结构如下  
```c
typedef struct Path
{
    NodeTag     type;
    NodeTag     pathtype;

    RelOptInfo *parent;
    AttrList    pathtarget;
    Cardinality rows;
    Cost        total_cost;
} Path;
```
其中`pathtype`表示路径的实际类型，`parent`表示路径所属表，`pathtarget`表示该路径返回的字段，`rows`表示该路径返回记录数，`total_coat`表示该路径所需要的代价。  

## 流程

- 调用`calSingleRelSelectivity`来预估执行后该表返回的记录数
- 创建顺序扫描的路径，并计算代价
- 如果有索引，则创建索引扫描的路径，并计算代价
- 比较所有路径的代价，选择出来最小的

### 顺序扫描的代价估计

顺序扫描的代价有cpu代价和磁盘代价组成  
cpu代价为cpu处理每条记录的代价 * 该表总条数，cpu处理单条记录的代价预设为0.01;  
磁盘代价为扫描每页的代价 * 该表的页数，扫描每页的代价预设为1.0  

### 索引扫描的代价估计

索引扫描分为两种情况，一种是简单条件，一种是复杂条件，用于连接的。两个情况计算方式相同，只是简单条件会作为路径比较最小代价，而复杂条件在动态规划的时候使用  
索引的代价分为cpu代价、磁盘代价、b+树代价  
cpu代价为cpu处理每条记录的代价 * 该表总条数，cpu处理单条记录的代价预设为0.01;  
磁盘代价为扫描每页的代价 * 该表的页数，扫描每页的代价预设为1.0。页数计算最大代价，为索引的页数 * 选择度 + 表的页数。  
b+树代价为数的高度 * 50 * cpu一次操作的代价和索引元组数对2取对数 * cpu一次操作的代价。  

## 连接的代价

连接的代价为外表扫描一次的代价 + 内表扫描一次的代价 + 外表的行数 * 内表的行数 * cpu处理一个元组的代价。如果外表的函数大于1，则还需要计算内表重新扫描的代价 * (外表数 - 1)  

## 计划转为执行迭代器

将投影和选择下推到叶子节点，最后来一次总的投影

## 参考
PostgreSQL 9.1版本源码  
PostgreSQL 16.0版本源码
