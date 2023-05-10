//
// Created by elias on 23-5-3.
//

#ifndef MICRODBMS_PRIMNODES_H
#define MICRODBMS_PRIMNODES_H

#include "nodes.h"
#include "ext.h"
#include "pathnodes.h"
#include "attnum.h"

typedef struct Expr
{
    NodeTag     type;
} Expr;

typedef struct OpExpr
{
    Expr    xpr;
    Oid     opno;

    List   *args;
} OpExpr;

typedef struct Var
{
    Expr        xpr;
    int         varno;
    AttrNumber  varattno;
} Var;

#endif //MICRODBMS_PRIMNODES_H
