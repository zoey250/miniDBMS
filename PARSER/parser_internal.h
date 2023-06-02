//parser_internal.h：REDBASE解析器的内部声明
#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "parser.h"

//实际使用double
typedef float real;

//提示
#define PROMPT  "\nMICROBASE >> "

//REL_ATTR：描述一个限定属性（relName.attrName）
typedef struct {
    char *relName;      /* relation name        */
    char *attrName;     /* attribute name       */
} REL_ATTR;

//ATTR_VAL：<属性，值>对
typedef struct {
    char *attrName;     /* attribute name       */
    enum AttrType valType;   /* type of value        */
    int valLength;      /* length if type = STRING */
    void *value;        /* value for attribute  */
} ATTR_VAL;

//所有可用的节点
typedef enum {
    N_SHOWDBS,
    N_CREATEDB,
    N_DROPDB,
    N_USEDB,
    N_SHOWTABLES,
    N_CREATETABLE,
    N_CREATEINDEX,
    N_DROPTABLE,
    N_DROPINDEX,
    N_LOAD,
    N_SET,
    N_HELP,
    N_PRINT,
    N_QUERY,
    N_INSERT,
    N_DELETE,
    N_UPDATE,
    N_RELATTR,
    N_CONDITION,
    N_RELATTR_OR_VALUE,
    N_ATTRTYPE,
    N_VALUE,
    N_RELATION,
    N_STATISTICS,
    N_LIST,
    N_ANALYZE,
} NODEKIND;


//解析树节点的结构
typedef struct node {
    NODEKIND kind;

    union {
        /* SM component nodes */
        struct {
            char *relname;
        } DB_OP;

        /* create table node */
        struct {
            char *relname;
            struct node *attrlist;
        } CREATETABLE;

        /* create index node */
        struct {
            char *relname;
            char *attrname;
        } CREATEINDEX;

        /* drop index node */
        struct {
            char *relname;
            char *attrname;
        } DROPINDEX;

        /* drop table node */
        struct {
            char *relname;
        } DROPTABLE;

        /* load node */
        struct {
            char *relname;
            char *filename;
        } LOAD;

        /* set node */
        struct {
            char *paramName;
            char *string;
        } SET;

        /* help node */
        struct {
            char *relname;
        } HELP;

        /* print node */
        struct {
            char *relname;
        } PRINT;

        /* QL component nodes */
        /* query node */
        struct {
            struct node *relattrlist;
            struct node *rellist;
            struct node *conditionlist;
        } QUERY;

        /* insert node */
        struct {
            char *relname;
            struct node *valuelist;
        } INSERT;

        /* delete node */
        struct {
            char *relname;
            struct node *conditionlist;
        } DELETE;

        /* update node */
        struct {
            char *relname;
            struct node *relattr;
            struct node *relorvalue;
            struct node *conditionlist;
        } UPDATE;

        /* command support nodes */
        /* relation attribute node */
        struct {
            char *relname;
            char *attrname;
        } RELATTR;

        /* condition node */
        struct {
            struct node *lhsRelattr;
            enum CompOp op;
            struct node *rhsRelattr;
            struct node *rhsValue;
        } CONDITION;

        /* relation-attribute or value */
        struct {
            struct node *relattr;
            struct node *value;
        } RELATTR_OR_VALUE;

        /* <attribute, type> pair */
        struct {
            char *attrname;
            char *type;
            int size;
            enum AttrSpec spec;
        } ATTRTYPE;

        /* <value, type> pair */
        struct {
            enum AttrType type;
            int  ival;
            real rval;
            char *sval;
        } VALUE;

        /* relation node */
        struct {
            char *relname;
        } RELATION;

        /* list node */
        struct {
            struct node *curr;
            struct node *next;
        } LIST;

        /* analyze node */
        struct {
            char *relname;
        } ANALYZE;
    } u;
} NODE;


//功能原型
NODE *newnode(NODEKIND kind);
NODE *show_dbs_node();
NODE *create_db_node(char *relname);
NODE *drop_db_node(char *relname);
NODE *use_db_node(char *relname);
NODE *show_tables_node();
NODE *create_table_node(char *relname, NODE *attrlist);
NODE *create_index_node(char *relname, char *attrname);
NODE *drop_index_node(char *relname, char *attrname);
NODE *drop_table_node(char *relname);
NODE *load_node(char *relname, char *filename);
NODE *set_node(char *paramName, char *string);
NODE *help_node(char *relname);
NODE *print_node(char *relname);
NODE *query_node(NODE *relattrlist, NODE *rellist, NODE *conditionlist);
NODE *insert_node(char *relname, NODE *valuelist);
NODE *delete_node(char *relname, NODE *conditionlist);
NODE *update_node(char *relname, NODE *relattr, NODE *value,
                  NODE *conditionlist);
NODE *relattr_node(char *relname, char *attrname);
NODE *condition_node(NODE *lhsRelattr, enum CompOp op, NODE *rhsRelattrOrValue);
NODE *value_node(enum AttrType type, void *value);
NODE *relattr_or_value_node(NODE *relattr, NODE *value);
NODE *attrtype_node(char *attrname, char *type, int size, enum AttrSpec spec);
NODE *relation_node(char *relname);
NODE *list_node(NODE *n);
NODE *prepend(NODE *n, NODE *list);
NODE *prepend_list(NODE *l, NODE* list);
NODE *analyze_table_node(char *relname);

#ifdef __cplusplus
extern "C" void reset_scanner(void);
extern "C" void reset_charptr(void);
extern "C" void new_query(void);
extern "C" RC   interp(NODE *n);
extern "C" int  yylex(void);
extern "C" int  yyparse(void);
#endif

#endif




