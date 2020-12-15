#include "cuckoo.h"

const int KV_NUM = 10000000;

CuckooEntry kv[KV_NUM+10];

inline void random_string(char* str, const int len){
    for(int i = 0; i < len; ++i)
        str[i] = '!' + rand()%93;
    str[len-1] = 0;
}

void create_kv(){
    srand((uint32_t)time(0));
    for(int i = 0; i < KV_NUM; ++i){
        random_string(kv[i].key, KEY_LEN);
        uint64_t x = 1;
        memcpy(kv[i].value, (const char*)&x, VAL_LEN*sizeof(char));
    }
}
