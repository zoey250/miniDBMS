//
// Created by elias on 23-5-2.
//

#include "list.h"
#include "paths.h"
#include "cost.h"

PathKeysComparison
compare_pathkeys(List *keys1, List *keys2)
{
    ListCell   *key1;
    ListCell   *key2;

    if (keys1 == keys2)
    {
        return PATHKEYS_EQUAL;
    }

    forboth(key1, keys1, key2, keys1)
    {
        PathKey    *pathkey1 = (PathKey *) lfirst(key1);
        PathKey    *pathkey2 = (PathKey *) lfirst(key2);

        if (pathkey1 != pathkey2)
        {
            return PATHKEYS_DIFFERENT;
        }
    }

    if (key1 != NULL)
    {
        return PATHKEYS_BETTER1;
    }
    if (key2 != NULL)
    {
        return PATHKEYS_BETTER2;
    }
    return PATHKEYS_EQUAL;
}
