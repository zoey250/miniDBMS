cmake_minimum_required(VERSION 3.5)
project(microDBMS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 -O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -O3 -std=c++14 -Wall")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FLEX)
find_package(BISON)
find_package(glog)
find_package(gflags)

# glog的依赖库
if (GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIR})
else()
    set(GLOG_LIBRARIES "-lglog")
endif()

# gflags的以来库
if (GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIR})
else()
    set(GFLAGS_LIBRARIES "-lgflags")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/PF)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/IX)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/QL)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/PARSER)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

BISON_TARGET(Parser PARSER/parse.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.cc COMPILE_FLAGS --defines=${CMAKE_CURRENT_BINARY_DIR}/y.tab.h)
FLEX_TARGET(Lexer PARSER/scan.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

file(GLOB C_SOURCE_FILES
        "*.h"
        "*.c"
        "PF/*.h"
        "PF/*.c"
        "RM/*.h"
        "RM/*.c"
        "IX/*.h"
        "IX/*.c"
        "SM/*.h"
        "SM/*.c"
        "QL/*.h"
        "QL/*.c"
        "PARSER/*.h"
        "PARSER/*.c"
        )

file(GLOB CPP_SOURCE_FILES
        "*.h"
        "*.cc"
        "PF/*.h"
        "PF/*.cc"
        "RM/*.h"
        "RM/*.cc"
        "IX/*.h"
        "IX/*.cc"
        "SM/*.h"
        "SM/*.cc"
        "QL/*.h"
        "QL/*.cc"
        "PARSER/*.h"
        "PARSER/*.cc"
        )
set(SOURCE_FILES
        ${C_SOURCE_FILES}
        ${CPP_SOURCE_FILES}
        ${BISON_Parser_OUTPUTS}
        ${FLEX_Lexer_OUTPUTS}
        )

#add_executable(dbcreate ${SOURCE_FILES} "dbcreate.cpp")
add_executable(microDBMS ${SOURCE_FILES} "redbase.cpp")
#add_executable(rm_test ${SOURCE_FILES} "rm_test.cpp")
#add_executable(ix_test ${SOURCE_FILES} "ix_test.cpp")

# 将gflog和gflags的库文件链接到项目
#target_link_libraries(dbcreate ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})
target_link_libraries(microDBMS ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})
#target_link_libraries(rm_test ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})
#target_link_libraries(ix_test ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})

