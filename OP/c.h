//
// Created by elias on 23-4-30.
//

#ifndef MICRODBMS_C_H
#define MICRODBMS_C_H

#include <stddef.h>
#include <stdbool.h>

#define FLEXIBLE_ARRAY_MEMBER

#define Max(x, y)   ((x) > (y) ? (x) : (y))
#define Min(x, y)   ((x) < (y) ? (x) : (y))

typedef unsigned int Index;

#ifndef HAVE_INT8
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
#endif

#ifndef HAVE_UINT8
typedef unsigned char uint8;
typedef unsigned short uin16;
typedef unsigned int uint32;
#endif

#ifndef HAVE_UINT64
typedef unsigned long int uint64;
#endif

#endif //MICRODBMS_C_H
