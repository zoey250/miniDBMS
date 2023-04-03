//nodes.c: 分配初始化解析树节点的函数

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include "../redbase.h"
#include "parser_internal.h"
#include "y.tab.h"

//可用于给定解析树的节点总数
#define MAXNODE     8192

static NODE nodepool[MAXNODE];
static int nodeptr = 0;

//reset_parser：在发生语法错误时重置扫描器和解析器
void reset_parser(void) {
    reset_scanner();
    nodeptr = 0;
}

static void (*cleanup_func)() = NULL;

//new_query：通过释放所有使用的资源为新查询做准备通过以前的查询。
void new_query(void) {
    nodeptr = 0;
    reset_charptr();
    if (cleanup_func != NULL)
        (*cleanup_func)();
}

void register_cleanup_function(void (*func)()) {
    cleanup_func = func;
}

//newnode：分配指定种类的新节点并返回指针成功。 错误返回NULL。
NODE *newnode(NODEKIND kind) {
    NODE *n;

    //如果用完了所有节点，则错误
    if (nodeptr == MAXNODE) {
        fprintf(stderr, "%s:%s out of memory\n", __FILE__, __FUNCTION__);
        exit(1);
    }

    //获取下一个节点
    n = nodepool + nodeptr;
    ++nodeptr;

    //初始化`kind'字段
    n -> kind = kind;
    return n;
}

//显示数据库
NODE *show_dbs_node() {
    return newnode(N_SHOWDBS);
}

//建库
NODE *create_db_node(char *relname) {
    NODE *n = newnode(N_CREATEDB);
    n->u.DB_OP.relname = relname;
    return n;
}

//删除数据库
NODE *drop_db_node(char *relname) {
    NODE *n = newnode(N_DROPDB);
    n->u.DB_OP.relname = relname;
    return n;
}

//打开数据库
NODE *use_db_node(char *relname) {
    NODE *n = newnode(N_USEDB);
    n->u.DB_OP.relname = relname;
    return n;
}

//显示表
NODE *show_tables_node() {
    return newnode(N_SHOWTABLES);
}

//建表
NODE *create_table_node(char *relname, NODE *attrlist) {
    NODE *n = newnode(N_CREATETABLE);

    n -> u.CREATETABLE.relname = relname;
    n -> u.CREATETABLE.attrlist = attrlist;
    return n;
}

//建索引
NODE *create_index_node(char *relname, char *attrname) {
    NODE *n = newnode(N_CREATEINDEX);

    n -> u.CREATEINDEX.relname = relname;
    n -> u.CREATEINDEX.attrname = attrname;
    return n;
}

//删除索引
NODE *drop_index_node(char *relname, char *attrname) {
    NODE *n = newnode(N_DROPINDEX);

    n -> u.DROPINDEX.relname = relname;
    n -> u.DROPINDEX.attrname = attrname;
    return n;
}

//删除表
NODE *drop_table_node(char *relname) {
    NODE *n = newnode(N_DROPTABLE);

    n -> u.DROPTABLE.relname = relname;
    return n;
}

//load
NODE *load_node(char *relname, char *filename) {
    NODE *n = newnode(N_LOAD);

    n -> u.LOAD.relname = relname;
    n -> u.LOAD.filename = filename;
    return n;
}

//set
NODE *set_node(char *paramName, char *string) {
    NODE *n = newnode(N_SET);

    n -> u.SET.paramName = paramName;
    n -> u.SET.string = string;
    return n;
}

//help
NODE *help_node(char *relname) {
    NODE *n = newnode(N_HELP);

    n -> u.HELP.relname = relname;
    return n;
}

//print
NODE *print_node(char *relname) {
    NODE *n = newnode(N_PRINT);

    n -> u.PRINT.relname = relname;
    return n;
}

//query
NODE *query_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist) {
    NODE *n = newnode(N_QUERY);

    n->u.QUERY.relattrlist = relattrlist;
    n->u.QUERY.rellist = rellist;
    n->u.QUERY.conditionlist = conditionlist;
    return n;
}

//insert
NODE *insert_node(char *relname, NODE *valuelist) {
    NODE *n = newnode(N_INSERT);

    n->u.INSERT.relname = relname;
    n->u.INSERT.valuelist = valuelist;
    return n;
}

//delete
NODE *delete_node(char *relname, NODE *conditionlist) {
    NODE *n = newnode(N_DELETE);

    n->u.DELETE.relname = relname;
    n->u.DELETE.conditionlist = conditionlist;
    return n;
}

//update
NODE *update_node(char *relname, NODE *relattr, NODE *relorvalue,
                  NODE *conditionlist) {
    NODE *n = newnode(N_UPDATE);

    n->u.UPDATE.relname = relname;
    n->u.UPDATE.relattr = relattr;
    n->u.UPDATE.relorvalue = relorvalue;
    n->u.UPDATE.conditionlist = conditionlist;
    return n;
}


//relattr_node：分配，初始化并返回指向新指针的指针
NODE *relattr_node(char *relname, char *attrname) {
    NODE *n = newnode(N_RELATTR);

    n -> u.RELATTR.relname = relname;
    n -> u.RELATTR.attrname = attrname;
    return n;
}

//condition_node：分配，初始化并返回指向新对象的指针
NODE *condition_node(NODE *lhsRelattr, CompOp op, NODE *rhsRelattrOrValue) {
    NODE *n = newnode(N_CONDITION);

    n->u.CONDITION.lhsRelattr = lhsRelattr;
    n->u.CONDITION.op = op;
    if (rhsRelattrOrValue) {
        // binary operator
        n->u.CONDITION.rhsRelattr =
            rhsRelattrOrValue->u.RELATTR_OR_VALUE.relattr;
        n->u.CONDITION.rhsValue =
            rhsRelattrOrValue->u.RELATTR_OR_VALUE.value;
    }
    return n;
}

// value_node：分配，初始化并返回指向新指针的指针
NODE *value_node(AttrType type, void *value) {
    NODE *n = newnode(N_VALUE);

    n->u.VALUE.type = type;
    switch (type) {
    case INT:
        n->u.VALUE.ival = *(int *)value;
        break;
    case FLOAT:
        n->u.VALUE.rval = *(float *)value;
        break;
    case STRING:
        n->u.VALUE.sval = (char *)value;
        break;
    }
    return n;
}

//relattr_or_valuenode：分配，初始化并返回指向的指针
NODE *relattr_or_value_node(NODE *relattr, NODE *value) {
    NODE *n = newnode(N_RELATTR_OR_VALUE);

    n->u.RELATTR_OR_VALUE.relattr = relattr;
    n->u.RELATTR_OR_VALUE.value = value;
    return n;
}

//attrtype_node：分配，初始化并返回指向新指针的指针
NODE *attrtype_node(char *attrname, char *type, int size, enum AttrSpec spec) {
    NODE *n = newnode(N_ATTRTYPE);

    n -> u.ATTRTYPE.attrname = attrname;
    n -> u.ATTRTYPE.type = type;
    n -> u.ATTRTYPE.size = size;
    n -> u.ATTRTYPE.spec = spec;
    return n;
}

//Relation_node：分配，初始化并返回指向新对象的指针
NODE *relation_node(char *relname) {
    NODE *n = newnode(N_RELATION);

    n->u.RELATION.relname = relname;
    return n;
}

// list_node：分配，初始化并返回指向新指针的指针
NODE *list_node(NODE *n) {
    NODE *list = newnode(N_LIST);

    list -> u.LIST.curr = n;
    list -> u.LIST.next = NULL;
    return list;
}

//将节点n放在列表的前面。
NODE *prepend(NODE *n, NODE *list) {
    NODE *newlist = newnode(N_LIST);

    newlist -> u.LIST.curr = n;
    newlist -> u.LIST.next = list;
    return newlist;
}

NODE *prepend_list(NODE *l, NODE *list) {
    NODE* r = l;
    assert(l->kind == N_LIST);
    while (l->u.LIST.next != NULL) {
        l = l->u.LIST.next;
        assert(l->kind == N_LIST);
    }
    l->u.LIST.next = list;
    return r;
}
