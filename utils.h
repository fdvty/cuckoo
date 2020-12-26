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
    for(uint64_t i = 0; i < KV_NUM; ++i){
        // random_string((char*)&kv[i].key, KEY_LEN);
        // kv[i].key &= 0x7fffffffffffffff;
        // kv[i].key += 1; 
        kv[i].key = i+13;
        kv[i].value = new uint64_t;
        *kv[i].value = i;
    }
}

void delete_kv(){
    for(int i = 0; i < KV_NUM; ++i)
        delete kv[i].value;
}
