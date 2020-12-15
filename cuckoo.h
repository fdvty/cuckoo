#ifndef CUCKOO_H
#define CUCKOO_H

#include <algorithm>
#include <cstring>
#include <string>
#include "murmur3.h"
#include <cassert>
#include "immintrin.h"

using namespace std;



#define KEY_LEN 8
#define VAL_LEN 8


struct CuckooEntry{
    char key[KEY_LEN];
    char value[VAL_LEN];

    CuckooEntry(){
        memset(key, 0, KEY_LEN*sizeof(char));
        memset(value, 0, VAL_LEN*sizeof(char));     
    }
    CuckooEntry(const CuckooEntry& e){
        memcpy(key, e.key, KEY_LEN*sizeof(char));
        memcpy(value, e.value, VAL_LEN*sizeof(char));
    }
    CuckooEntry& operator=(const CuckooEntry& e){
        memcpy(key, e.key, KEY_LEN*sizeof(char));
        memcpy(value, e.value, VAL_LEN*sizeof(char));
        return *this;
    }
    bool operator==(const CuckooEntry& e){
        if(memcmp(key, e.key, KEY_LEN) == 0)
            return true;
        return false;
    }
    bool operator!=(const CuckooEntry& e){
        if(memcmp(key, e.key, KEY_LEN) != 0)
            return true;
        return false;
    }
}empty_entry;

#define MAX_THRESHOLD 100

class Cuckoo{
private:
    CuckooEntry *table[2]; 
    int size[2];            
    int width[2];           
    int threshold;  
    uint32_t count_item;         

    int h[2];
    CuckooEntry e_kick[MAX_THRESHOLD];
    uint32_t hash_kick[MAX_THRESHOLD];

    uint32_t hashseed[3];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 35;

        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
        count_item = 0;
        // initialize two sub-tables
        for(int i = 0; i < 2; ++i){
            table[i] = new CuckooEntry[size[i]];
            memset(table[i], 0, sizeof(CuckooEntry)*size[i]);
        }
        // initialize hash funcitons
        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            hashseed[i] = seed;
        }
        hashseed[2] = rand()%MAX_PRIME32;
        assert(threshold < MAX_THRESHOLD);
    }

    ~Cuckoo(){
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }


private:
    inline int hash_value(const char* key, int t){
        return MurmurHash3_x86_32(key, KEY_LEN, hashseed[t]) % (size[t] - width[t] + 1);
    }

    inline int default_table(const char* key){
        return (MurmurHash3_x86_32(key, KEY_LEN, hashseed[2]) & 0x3) ? 0 : 1;
    }

    inline bool query_table(const char* key, int t, char* ret_value = NULL, const char* cover_value = NULL){
        if(h[t] == -1)
            h[t] = hash_value(key, t);
        int position = h[t];

        for(int i = position; i < position + width[t]; ++i)
            if(memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                if(cover_value != NULL){
                    // *需要修改* 此处为和旧值做加法，这里简单写成转换成uint64相加，如果value为指针的话需要修改为把指针指向的元素相加
                    uint64_t result = *(uint64_t*)table[t][i].value + *(uint64_t*)cover_value;
                    memcpy(table[t][i].value, &result, VAL_LEN*sizeof(char));
                    // memcpy(table[t][i].value, value, VAL_LEN*sizeof(char));
                }
                if(ret_value != NULL)
                    memcpy(ret_value, table[t][i].value, VAL_LEN*sizeof(char));
                return true;
            }
        
        return false;
    }


    inline bool insert_table(const CuckooEntry &e, int t){
        if(h[t] == -1)
            h[t] = hash_value(e.key, t);
        int position = h[t];

        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i] == empty_entry){
                memcpy(table[t][i].key, e.key, KEY_LEN*sizeof(char));
                memcpy(table[t][i].value, e.value, VAL_LEN*sizeof(char));
                count_item++; 
                return true;
            }

        return false;
    }

public:
    bool query(const char *key, char *ret_value = NULL){
        h[0] = h[1] = -1;
        if(query_table(key, 0, ret_value, NULL) || query_table(key, 1, ret_value, NULL))
            return true;
        int t = default_table(key);
        if(ret_value != NULL)
            memcpy(ret_value, table[t][h[t]].value, VAL_LEN*sizeof(char));
        return false;
    }

    inline bool insert_general(const CuckooEntry &e){
        // 之前没插入过
        // 首先看是否有空位置，尝试直接插入
        h[0] = h[1] = -1;
        if(insert_table(e, 0) || insert_table(e, 1))
            return true;

        // 尝试直接插入失败，尝试进行kick调整
        CuckooEntry e_insert = e;
        CuckooEntry e_tmp;

        for(int k = 0; k < threshold; ++k){
            int t = k%2;
            if(k != 0){
                h[t] = hash_value(e_insert.key, t);
                if(insert_table(e_insert, t))
                    return true;
            }
            e_tmp = table[t][h[t]];
            table[t][h[t]] = e_insert;
            e_insert = e_tmp;
            // 记录这次被踢出去的元素和它的哈希值，方便回溯
            hash_kick[k] = h[t];
            e_kick[k] = e_tmp;
        }

        // kick调整失败，进行回溯，返回插入e之前的状态
        for(int k = threshold-1; k >= 0; --k){
            int t = k%2; 
            table[t][hash_kick[k]] = e_kick[k];
        }

        return false;
    }

    inline bool insert_enforce(const CuckooEntry &e){
        // // 尝试查询，如果查到了直接相加
        // if(query_flag && (query_table(e.key, 0, NULL, h[0], e.value) || query_table(e.key, 1, NULL, h[1], e.value)))
        //     return true;
        // // 尝试插入，如果有空位直接插入。如果这个元素已经插入过，则不应该尝试插入
        // if(insert_flag && (insert_table(e, 0, h[0]) || insert_table(e, 1, h[1])))
        //     return true;
        // 直接累加（应该保证这个元素已经被插入过）
        int t = default_table(e.key);
        if(h[t] == -1)
            h[t] = hash_value(e.key, t);
        uint64_t result = *(uint64_t*)table[t][h[t]].value + *(uint64_t*)e.value;
        memcpy(table[t][h[t]].value, &result, VAL_LEN*sizeof(char));
        return false;
    }

    bool insert(const CuckooEntry &e, bool first_come = true){
        double lf = (double)count_item / (size[0] + size[1]);   // load factor
        h[0] = h[1] = -1;
        if(!first_come){
            if(query_table(e.key, 0, NULL, e.value) || query_table(e.key, 1, NULL, e.value))
                return true;
            insert_enforce(e);
        }
        else if(lf <= 0.9){
            if(insert_general(e))
                return true;
            insert_enforce(e);
        }
        else{
            if(insert_table(e, 0) || insert_table(e, 1))
                return true;
            insert_enforce(e);
        }
        return true;
    }

public:
    double loadfactor(){
        int full = 0;
        for(int i = 0; i < 2; ++i){
            int tmp = 0;
            for(int j = 0; j < size[i]; ++j)
                if(table[i][j] != empty_entry)
                    tmp += 1;
            full += tmp;
            printf("table %d, load factor %lf\n", i, (double)tmp/size[i]);
        }
        assert(full == count_item);
        return (double)full/(size[0] + size[1]);
    }
};

#endif
