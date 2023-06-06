#include <iostream>
#include <map>
#include <array>
#include "../redbase.h"
#include "../OP/paths.h"

#ifndef OP_DYNAMIC_H
#define OP_DYNAMIC_H

// class Head {                //头的类，它里面主要有一个relList来标识都有哪些关系
// public:
//     Head(int i) {
//         relList = (unsigned char) i;
//     }
  
//     bool operator<(const Head& other) const {
//         return relList < other.relList;
//     }

//     void print(){
//         std::cout << relList;
//     }

//     std::size_t getHash() const {
//         // Implement a simple hash function by casting the relList variable to size_t
//         return static_cast<std::size_t>(relList);
//     }
    
//     unsigned char relList;
// };

// struct HeadHash {
//   std::size_t operator()(const Head& head) const {
//     return head.getHash();
//   }
// };

Path* doDynamic(PlannerInfo* root);

#endif