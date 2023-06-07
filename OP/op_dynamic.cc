#include <vector>
#include "op_dynamic.h"
#include "joinpath.h"

unsigned char generateBinaryKey(int i)
{
    return 1u << (i - 1);
}

bool can_join(unsigned char left,unsigned char right){
    int mask = 0b1;
    bool flag = true;
    while (left > 0 || right > 0) {
        if ((left & mask) == 1 && (right & mask) == 1) {
            flag = false;
            break;
        }
        left >>= 1;
        right >>= 1;
    }
    return flag;
}

Path* doDynamic(PlannerInfo* root){
    int levels_needed = root->simple_rel_array_size;
    std::vector<std::map<unsigned char, Path*>> bestJoin(levels_needed);           //最佳连接表
    std::vector<std::map<unsigned char, NestPath*>> bestJoinNest(levels_needed);           //最佳连接表
    for(int i=0; i< levels_needed;i++){             //生成第一层最佳连接表
        bestJoin[0][generateBinaryKey(i+1)] = root->simple_rel_array[i]->cheapest_total_path;
        printf("%d\n",i);
    }

    for(int i=1; i < levels_needed ; i++){
        for (auto it = bestJoin[i-1].begin(); it != bestJoin[i-1].end(); ++it) {
            printf("第%d层\n",i);
            unsigned char left_key = it->first;
            Path* left_value = it->second;
            printf("%d\n",left_key);

            int j = 0;
            for (auto it_o = bestJoin[0].begin(); it_o != bestJoin[0].end(); ++it_o) {
                unsigned char right_key = it_o->first;
                Path* right_value = it_o->second;
                if(can_join(left_key,right_key)){
                    printf("%d-%d=%d\n",left_key,right_key,left_key|right_key);
                    NestPath* join_path = create_nestloop_path(left_value,right_value);
                    if(bestJoin[i].find(left_key|right_key) != bestJoin[i].end()){      //如果已经计算过的连接方式
                        Path* tmp = (Path*) join_path; 
                        if(tmp->total_cost < bestJoin[i][left_key|right_key]->total_cost){
                            bestJoin[i][left_key|right_key] = (Path*) join_path;
                            bestJoinNest[i][left_key|right_key] = join_path;
                        }
                    }else{
                        bestJoin[i][left_key|right_key] = (Path*) join_path;
                        bestJoinNest[i][left_key|right_key] = join_path;
                    }

                    ListCell   *lc;
                    foreach(lc, root->simple_rel_array[j]->pathlist)
                    {
                        Path   *p = (Path *) lfirst(lc);
                        if (p->pathtype == T_IndexScan)
                        {
                            IndexPath  *iPath = (IndexPath *) p;
                            if (iPath->indexinfo->complexconditions.empty())
                            {
                                continue;
                            }

                            for (int k = 0; k < iPath->indexinfo->complexconditions.size(); ++k)
                            {
                                QL_Condition condition = iPath->indexinfo->complexconditions[k];
                                if (is_leaf(left_value))
                                {
                                    for (int l = 0; l < left_value->parent->complexconditions.size(); ++l)
                                    {
                                        if (condition == left_value->parent->complexconditions[l])
                                        {
                                            NestPath *index_join_path = create_nestloop_path(left_value, p);
                                            if (((Path *) index_join_path)->total_cost < bestJoin[i][left_key | right_key]->total_cost)
                                            {
                                                bestJoin[i][left_key | right_key] = (Path *) index_join_path;
                                            }
                                        }
                                    }
                                }
                                if (is_join(left_value))
                                {
                                    NestPath   *nPath = (NestPath *) left_value;
                                    for (int l = 0; l < nPath->jpath.outercomplexconditions.size(); ++l)
                                    {
                                        if (condition == nPath->jpath.outercomplexconditions[l])
                                        {
                                            NestPath *index_join_path = create_nestloop_path(left_value, p);
                                            if (((Path *) index_join_path)->total_cost < bestJoin[i][left_key | right_key]->total_cost)
                                            {
                                                bestJoin[i][left_key | right_key] = (Path *) index_join_path;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                j++;
            }
        }
        // 输出到控制台
        // printf("%d",key);
    }
    for (auto it_o = bestJoin[levels_needed-1].begin(); it_o != bestJoin[levels_needed-1].end(); ++it_o) {
        unsigned char left_key = it_o->first;
        Path* left_value = it_o->second;
        return left_value;
    }
    return root->simple_rel_array[root->simple_rel_array_size-1]->cheapest_total_path;
}