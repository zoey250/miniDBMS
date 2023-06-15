**目录**

[TOC]

# 摘要

`dev分支`包含了全部功能的代码

我们实现了基于代价的查询优化的功能, 主要内容是确定是否使用索引以及连接顺序选择。它包括四个子模块：统计信息收集、选择度估计、代价估计、路径选择与计划生成。各个子模块的关系如下：



<img src="%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612131328836.png" alt="image-20230612131328836" style="zoom:33%;" />



1. 统计信息收集模块收集数值型数据（INT）的直方图信息， 并将其存储在`relstatistics`中，表中记录了每张表每列的直方图桶数以及桶信息；
2. 选择度估计模块利用收集来的统计信息进行单表的以及多表的选择度估计；
3. 代价估计模块利用选择度估计的结果估计顺序扫描、索引扫描以及连接的路径的代价；
4. 路径选择与计划生成模块利用代价估计模块中的路径代价生成最佳的执行计划。

我们的项目目前托管在 https://github.com/zoey250/miniDBMS

# 运行说明

我们提供了 `CMakeLists.txt` 文件用于`cmake`编译。运行时，可直接使用`clion`编译器执行，或先通过`cmake`编译，然后通过`./microDBMS`执行，功能语句如下：

```c++
//选择数据库
use orderDB;
//开启打印查询计划功能
queryplans on;
//统计信息收集, 收集tablename对应的直方图
analyze table tablename;
//查看统计信息收集结果
select * from relstatistics;
//测试查询语句
SELECT customer.name,book.title,orders.quantity FROM customer,book,orders WHERE
orders.customer_id=customer.id AND orders.book_id=book.id AND orders.quantity > 8;
```





# 模块接口实现说明

查询优化模块代码放在`OP`文件夹下，统计信息收集模块代码放在`statistics`文件夹下。

接下来详细介绍各个子模块，构造函数、析构函数、拷贝构造函数等就不再赘述。

## 统计信息收集

### 方案

添加关键字analyze，采样统计一张表，目前实现了整型统计。采用蓄水池抽样法进行采样，使用列等深直方图，然后将这些采样信息保存到统计表`relstatistics`中。统计表的结构如下：

![image-20230612105416012](%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612105416012.png)

从左到右依次是：表名、列名、该列直方图对应的桶数以及统计信息。

### 原理

蓄水池采样法解决的是从一个大数据量 N 中抽取出 M 个不重复的数据，例如从 100000 份调查报告中抽取 1000 份进行统计。它能够在只遍历一次数据的情况下，随机抽取出指定数量的不重复数据。 
蓄水池采样法的流程如下  ：

- 假设数据序列的规模为 n，需要抽取的数量为 m。  
- 首先构建一个容量为 m 的蓄水池数组，将序列的前 m 个元素放入蓄水池中。  
- 对于之后的元素，假设该元素为第 i 个（i >= m），在 [0, i] 中取得随机数 a，若 a 落在 [0, m-1] 范围内，那么该元素替换掉原来蓄水池中的第 a 个元素。  
- 在遍历完数据序列后，蓄水池中剩下的元素即为要抽取的样本  

其证明方法如下：

- 当 i < m 时，元素进入水池的概率为 1，这个不难理解，下面看元素不被替换的概率，当遍历到第 m 个元素时，替换池中元素的概率为 m/(m+1)，池中第 i 个元素被替换的概率为 1/m，所以总的来说，遍历到第 m 个元素时，第 i 个元素不被替换的概率为 1 - (m/(m+1) 1/m) = m/(m+1)。依次类推，当遍历到第 j 个元素时（j >= m），第 i 个元素不被替换的概率为 j/(j+1)。所以当遍历完 n 个元素时，第 i 个元素不被替换的概率为 m/(m+1) (m+1)/(m+2) … (n-1)/n = m/n。总的来说，当 i < m 时，第 i 个元素被抽取到的概率为 1 m/n = m/n。  
- 当 i >= m 时，在 [0, i] 中抽取随机数 d，如果 d < m，则替换掉池中的第 d 个元素，所以此时元素可以进入水池的概率为 m/(i+1)。元素进入到蓄水池后，由上面的证明可以得知，元素不被替换掉的概率为 (i+1)/n。所以总的来说，当 i >= m 时，第 i 个元素被抽取到的概率为 m/(i+1) * (i+1)/n = m/n。  

综上，可知遍历完一遍后，所以元素被抽取到的概率都为 m/n，即抽样是公平的。

### 流程

- 选择数据库

  - 检查当前数据库下时候有统计信息表`relstatistics`，如果没有，则创建表（调用`SM_Manager::CreateTable`）。  

    | 属性     | 类型              | 名字      |
    | -------- | ----------------- | --------- |
    | 表名     | char(MAXNAME + 1) | relname   |
    | 字段名   | char(MAXNAME + 1) | attrname  |
    | 桶的个数 | int(5)            | bucketnum |
    | 统计信息 | char(100,000)     | value     |

- 选择要分析的表

  - 检查表是否存在
  - 使用`SM_Manager::GetRelEntry`来获取元组数
  - 打开表，创建一个全表扫描的迭代器（仿照`QL:Manager::Select`流程）
  - 蓄水池采样，采样个数 = ceil(元组数 / 100)（待定，可能用一个map来记录，参考`mimalloc`的`bin`）
  - 使用`SM_Manager::GetDataAttrInfo`来获取属性列
  - 直方图的桶个数 = ceil(采样个数 / 20) (待定)
  - 各个整型字段排序，等深放入到桶中。
    - 如果 V 等于上一个值，那么把 V 放在与上一个值同一个桶里，无论桶是不是已经满，这样可以保证每个值只存在于一个桶中。 
    - 如果不等于，那么判断当前桶是否已经满，如果不是的话，就直接放入当前桶，否则的话，就放入下一个桶。
  - 查看统计表中是否有该表的统计信息
    - 如果没有，将桶的值范围写入到表中（仿照`QL_Manager::Insert`）
    - 如果有，则更新（`QL_Manager::Update`）
    - 实际写代码的时候，考虑到Delete也是全表扫描，就暴力了些，直接先删除再插入了

### 使用方法

在选择数据库之后，执行 `analyze table tablename;`即可对表取样，并将结果保存在`relstatistics`表中，使用`select * from relstatistics;`即可查到采样信息。  

## 选择度估计

### 方案

进行单表选择度以及多表连接的选择度估计。当有直方图统计信息时，使用直方图进行估计，否则使用公式进行估计。

下面是使用直方图和未使用直方图的选择度对比：

- ​	orders表一共: 22734 tuple(s).经过`select * from orders where quantity>8;`查询后：4088 tuple(s). 其真实选择度为: 0.179

- ​	不使用直方图时的选择度为：0.333，使用直方图时的选择度见下图：为0.25，可见直方图能够使选择度估计更为精准。

  ​	<img src="%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612010746441.png" alt="image-20230612010746441" style="zoom: 80%;" />

### 原理

#### 选择运算大小的估算

**利用直方图时**

首先将统计信息加载到内存，加载内存操作会在每次analyze之后执行并更新

对`=, <, <=, >, >=, !=`进行分情况讨论并根据桶的位置、桶的左右边界值、桶的数量以及最大最小值计算选择度，等深直方图中每个桶的选择度是 1/桶的个数

在计算时考虑了桶的左右值相等、查找值落在桶的边界上、查找值未出现在统计信息中等情况

**无直方图时**

V(R,A)为关系表R属性A中不同值的个数

1. 等值运算: 假设均匀分布

   选择度 =1 / V(R, A), 若没有V(R,A),  假设V(R, A) = 10 

2. 范围运算(<,>,≥,≤)

   选择度 = 1/3

3. 不等运算

   选择度 = (1-1/V(R,A))

4. 符合条件的选择运算(由于小DBMS中只实现了and，所以只考虑and, 不考虑or)

   假设条件间相互独立, 选择率 P(A and B) = P(A)*P(B)

#### 连接运算大小的估算

根据值集包含和值集保持假设进行估算, 选择率可以依次往上传递

<img src="%E6%9F%A5%E8%AF%A2%E4%BC%98%E5%8C%96.assets/image-20230516162839607.png" alt="image-20230516162839607" style="zoom: 25%;" />

R表和S表进行等值连接或自然连接：

选择度 = 1/max(V(R,Y),V(S,Y))

### 接口

```C++
double calSingleRelSelectivity(const std::vector<QL_Condition>& conditions)
```

1. 首先，获取条件列表的大小（即条件的数量）并初始化选择度为1.0。

2. 如果条件数量大于等于1，则遍历条件列表。

3. 对于每个条件，首先获取右操作数（`rhsValue`）的值。

4. 如果条件的左操作数类型为整型（`INT`）且统计数据不为空，则使用直方图来计算选择度。

   a. 遍历统计数据列表，查找与条件的左操作数（`cond.lhsAttr`）匹配的关系名和属性名。

   b. 找到匹配的统计数据后，获取桶的数量、目标值所在的桶位置（`bucketPosition`）以及桶的左右边界值。

   c. 如果桶的数量为0，则输出错误信息并返回-1。

   d. 根据条件的运算符（`cond.op`）进行不同的计算。

5. 如果条件的左操作数类型不是整型或统计数据为空，则使用默认的选择度计算。

6. 完成条件列表的遍历后，返回最终的选择度。

```c++
double calJoinSelectivity(double leftSelectivity, double rightSelectivity)
```

1. 首先，获取左侧关系的选择度和右侧关系的选择度作为输入参数。
2. 在连接操作中，连接后关系的选择度通常是两个关系选择度的较小值。因此，使用`std::min`函数来获取左侧选择度和右侧选择度的最小值，并将其赋值给`joinSelectivity`。
3. 最后，返回连接后关系的选择度（`joinSelectivity`）作为函数的输出。

## 代价估计

### 原理

#### 扫描代价

**顺序扫描的代价估计**

顺序扫描的代价由cpu代价和磁盘代价组成  

* cpu代价为cpu处理每条记录的代价 * 该表总条数，cpu处理单条记录的代价预设为0.01;  

* 磁盘代价为扫描每页的代价 * 该表的页数，扫描每页的代价预设为1.0  

**索引扫描的代价估计**

索引扫描分为两种情况，一种是简单条件，一种是复杂条件，用于连接的。两个情况计算方式相同，只是简单条件会作为路径比较最小代价，而复杂条件在动态规划的时候使用 
索引的代价分为cpu代价、磁盘代价、b+树代价 

* cpu代价为cpu处理每条记录的代价 * 该表总条数，cpu处理单条记录的代价预设为0.01;  
* 磁盘代价为扫描每页的代价 * 该表的页数，扫描每页的代价预设为1.0。页数计算最大代价，为索引的页数 * 选择度 + 表的页数
* b+树代价为数的高度 * 50 * cpu一次操作的代价和索引元组数对2取对数 * cpu一次操作的代价

#### 连接代价

连接的代价为：外表扫描一次的代价 + 内表扫描一次的代价 + 外表的行数 * 内表的行数 * cpu处理一个元组的代价

如果外表的行数大于1，则还需要计算内表重新扫描的代价 * (外表数 - 1)  

### 接口

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

```c++
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

### 流程

在代价估计部分会直接选择扫描时使用或者不使用索引，步骤如下：

1. 调用`calSingleRelSelectivity`来预估执行后该表返回的记录数

2. 创建顺序扫描的路径，并计算代价

3. 如果有索引，则创建索引扫描的路径，并计算代价

4. 比较所有路径的代价，选择出来最小的

计划转为执行迭代器：将投影和选择下推到叶子节点，最后来一次总的投影

## 路径选择与计划生成

### 原理

在miniDBMS中，我们使用动态规划来枚举构建一个左深树，进行连接顺序的选择，由代价估计的部分提供基础支撑

### 接口

接口信息见op_dynamic.h

```c++
Path* doDynamic(PlannerInfo* root)				//进行动态规划获取最佳连接树
```

连接顺序的选择的入参为PlannerInfo，返回一个Path的树型路径，该路径即为最优的连接路径。对应的结构与代价估计部分是同一类型，就不多介绍了。

```c++
NestPath* create_nestloop_path(Path *outer_path, Path *inner_path)				//计算左关系和右关系的连接代价等属性
```

输入左path和右path，输出NestPath。这里的NestPath相当于继承Path，也可以强制类型转换后递归的作为入参数。

### 数据结构

std::vector<std::map<unsigned char, Path*>> bestJoin(levels_needed);

bestJoin为一个Map数组：Map当中key为关系的二进制表示，value为对应关系的path



### 流程

* 获取root->simple_rel_array_size;
* 生成第一层最佳连接表
* for(2:左深树的层数)：                     //构建接下来的层i
  * for(bestJoin[i-1].begin():bestJoin[i-1].end()):						//遍历第i层下面一层
    * for(bestJoin[0].begin():bestJoin[0].end()):					//遍历第0层，将i-1层与0层进行左连接
      * if(can_join(left_key,right_key)):								//如果左关系的二进制表示和右关系的二进制表示每一位上都不同为1
        * NestPath* join_path = create_nestloop_path(left_value,right_value);		//计算连接代价等属性
        * 与key相同的bestJoin[key]对比，保留代价最小的Path
      * end
    * end
  * end
* end
* return Path;

# 实验结果

publisher 5000条数据;customer 7000条数据;book 45770条数据;orders 22734条数据

book>orders>customer>publisher

执行下列测试语句：

```sql
SELECT customer.name,book.title,orders.quantity FROM customer,book,orders WHERE
orders.customer_id=customer.id AND orders.book_id=book.id AND orders.quantity > 8;
```

在QL_Manager::Select的开始和结束位置记录时间戳，最后输出查询执行时间

* 未优化前

  * 执行时间为： 429503ms；

    <img src="%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612124454701.png" align="left" alt="image-20230612124454701" style="zoom:80%;" />

  

  

  * 执行计划为如下

    ![image-20230612121001576](%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612121001576.png)

* 优化后

  * 执行时间为：207231ms；

    <img src="%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612124541598.png" align="left" alt="image-20230612124541598" style="zoom:80%;"/>

  

  

  * 执行计划如下左图所示，右图为PG关闭小DBMS中没有的功能后的执行计划，二者相同

    ![image-20230612122015942](%E9%AB%98%E7%BA%A7%E6%95%B0%E6%8D%AE%E5%BA%93%E7%B3%BB%E7%BB%9F%E6%96%87%E6%A1%A3.assets/image-20230612122015942.png)

# 参考文献等

* **总体设计及路径选择和代码生成**

  人大金仓数据库内核培训查询优化部分

* **统计信息**

  [TiDB 源码阅读系列文章（十二）统计信息（上）](https://cn.pingcap.com/blog/tidb-source-code-reading-12)  
  [sample.go](https://github.com/pingcap/tidb/blob/5cc1c3b39eef1596d9432fe5db74ee02b83be46b/statistics/sample.go#L168)

* **选择度估计**

  MOOC：哈工大 数据库系统（下）：管理与技术
  Gregory Piatetsky-Shapiro：Accurate Estimation of the Number of Tuples Satisfying a Condition.[SIGMOD Conference 1984]: 256-276

* **代价估计**

  PostgreSQL 9.1版本源码  
  PostgreSQL 16.0版本源码
