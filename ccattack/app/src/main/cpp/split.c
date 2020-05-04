#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//返回一个 char *arr[], size为返回数组的长度
char **split(char sep, const char *str, size_t *size)
{
    int count = 0, i;
    for(i = 0; i < strlen(str); i++)
    {
        if (str[i] == sep)
        {
            count ++;
        }
    }
    char **ret = (char**)calloc(++count, sizeof(char *));
    int lastindex = -1;
    int j = 0;
    for(i = 0; i < strlen(str); i++)
    {
        if (str[i] == sep)
        {
            ret[j] = (char*)calloc(i - lastindex, sizeof(char)); //分配子串长度+1的内存空间
            memcpy(ret[j], str + lastindex + 1, i - lastindex - 1);
            j++;
            lastindex = i;
        }
    }
    //处理最后一个子串
    if (lastindex <= strlen(str) - 1)
    {
        ret[j] = (char*)calloc(strlen(str) - lastindex, sizeof(char));
        memcpy(ret[j], str + lastindex + 1, strlen(str) - 1 - lastindex);
        j++;
    }
    *size = *size+j;
    return ret;
}
