# 统计信息文档
## 方案
单独写一个分析程序（添加关键字要学bison、flex）采样统计一张表，采用蓄水池抽样法进行采样，使用列等深直方图，然后将这些采样信息保存到要给统计表中`relstatistics`。  
目前先准备整形统计。

## 流程

- 选择数据库
  - 检查当前数据库下时候有统计信息表`relstatistics`，如果没有，则创建表（调用`SM_Manager::CreateTable`）。  

    | 属性 | 类型 | 名字 |
    | -- | -- | -- |
    | 表名 | char(MAXNAME + 1) | relname |
    | 字段名 | char(MAXNAME + 1) | attrname |
    | 桶的个数 | int(5) | bucketnum |
    | 统计信息 | char(100,000) | value |

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

## 引用
[TiDB 源码阅读系列文章（十二）统计信息（上）](https://zhuanlan.zhihu.com/p/39139693)  
[sample.go](https://github.com/pingcap/tidb/blob/5cc1c3b39eef1596d9432fe5db74ee02b83be46b/statistics/sample.go#L168)

## 问题 & 优化
- [ ] 一条记录最大是`4096 - sizeof(int)`导致了统计信息的长度受限，也就是对桶的数量有限制。考虑查表的方式，根据元组数来调整桶的个数
- [ ] 单独一个程序虽然也是往数据库中插入记录，但是要想数据库正常使用，可能需要重启数据库，因为有buffer，有不一致的问题。或者有命令可以让数据库清空buffer。可能会考虑添加关键字，先尝试一下

