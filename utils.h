#include "cuckoo.h"

const int KV_NUM = 10000000;

Hash_entry kv[KV_NUM+10];

inline void random_string(char* str, const int len){
    for(int i = 0; i < len; ++i)
        str[i] = '!' + rand()%93;
    str[len-1] = 0;
}

void create_kv(){
    srand((uint)time(0));
    for(int i = 0; i < KV_NUM; ++i){
        random_string(kv[i].key, KEY_LEN);
        random_string(kv[i].value, VAL_LEN);
    }
}

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress (double percentage)
{
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf ("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush (stdout);
}