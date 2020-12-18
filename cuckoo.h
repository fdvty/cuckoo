#ifndef _CUCKOO_H_
#define _CUCKOO_H_

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
    uint64_t key;
    uint64_t* value;
    uint64_t freq; 

    CuckooEntry(){
        key = 0;
        value = 0; 
        freq = 0;   
    }
    CuckooEntry(const CuckooEntry& e){
        key = e.key;
        value = e.value;
        freq = e.freq;
    }
    CuckooEntry& operator=(const CuckooEntry& e){
        key = e.key;
        value = e.value;
        freq = e.freq;
        return *this;
    }
    bool operator==(const CuckooEntry& e){
        if(key == e.key)
            return true;
        return false;
    }
    bool operator!=(const CuckooEntry& e){
        if(key != e.key)
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

    CuckooEntry ev_kick[MAX_THRESHOLD];
    uint32_t positionv_kick[MAX_THRESHOLD];

    int epoch_kick;
    int t_kick;
    int position_kick;
    int min_freq;

    uint32_t hashseed[2];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 25;

        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
        count_item = 0;

        table[0] = new CuckooEntry[size[0] + size[1]];
        memset(table[0], 0, sizeof(CuckooEntry)*(size[0] + size[1]));
        table[1] = table[0] + size[0];

        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            hashseed[i] = seed;
        }
        assert(threshold < MAX_THRESHOLD);
    }

    ~Cuckoo(){
        delete [] table[0];
    }


private:
    inline int hash_value(uint64_t key, int t){
        return MurmurHash3_x86_32((const char*)&key, KEY_LEN, hashseed[t]) % (size[t] - width[t] + 1);
    }

    inline bool query_table(uint64_t key, int t, uint64_t* ret_value = NULL){
        int position = hash_value(key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].key == key){
                if(ret_value != NULL)
                    *ret_value = *table[t][i].value;
                table[t][i].freq++;
                return true;
            }
        return false;
    }

    inline bool update_table(uint64_t key, int t, uint64_t delta){
        int position = hash_value(key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].key == key){
                if(delta != 0)
                    *table[t][i].value += delta; 
                return true;
            }
        return false;
    }


    inline bool insert_table(const CuckooEntry &e, int t, int position = -1, int epoch_now = 0){
        if(position == -1)
            position = hash_value(e.key, t);
        for(int i = position; i < position + width[t]; ++i){
            if(table[t][i] == empty_entry){
                table[t][i] = e;
                count_item++; 
                return true;
            }
            if(table[t][i].freq < min_freq){
                min_freq = table[t][i].freq;
                epoch_kick = epoch_now;
                t_kick = t;
                position_kick = i;
            }
        }

        return false;
    }

public:
    bool query(uint64_t key, uint64_t* ret_value = NULL){
        if(query_table(key, 0, ret_value) || query_table(key, 1, ret_value))
            return true;
        return false;
    }

    // uint64_t* delta (add *delta to existing value)
    bool SGD_update(const CuckooEntry &e){
        if(update_table(e.key, 0, *e.value) || update_table(e.key, 1, *e.value))
            return true;
        return false;
    }

    // uint64_t* value (store this pointer)
    // 返回true说明插入成功，返回false说明替换出去了一个最频率小的

    // !!这里并没有对踢出去的KVpair的value指针做特殊处理（内存释放），实际应用时要做一些修改
    bool insert(const CuckooEntry &e, uint64_t* ret_value = NULL){
        min_freq = 0x3f3f3f3f;

        int position = hash_value(e.key, 0);
        if(insert_table(e, 0, position, 0) || insert_table(e, 1, -1, 0))
            return true;

        CuckooEntry e_insert = e;
        CuckooEntry e_tmp;

        int e_position = position;

        for(int k = 0; k < threshold; ++k){
            int t = k%2;
            if(k != 0){
                position = hash_value(e_insert.key, t);
                if(insert_table(e_insert, t, position, k))
                    return true;
            }
            e_tmp = table[t][position];  // k=0 时的postion上面已经算好
            table[t][position] = e_insert;
            e_insert = e_tmp;

            positionv_kick[k] = position; // 这里记的是每一轮**被踢出去**的那个元素
            ev_kick[k] = e_tmp;  //这里记得是每一轮被踢出去的那个元素的位置
        }
    

        for(int k = threshold-1; k > epoch_kick; --k){  // 把epoch_kick以后的表还原成插入之前的状态
            int t = k%2; 
            table[t][positionv_kick[k]] = ev_kick[k];
        }
        CuckooEntry e_kick;
        if(epoch_kick == 0 && t_kick == 1){ // 把第一轮的表也还原成插入之前的状态，把e插到table[1][position_kick]
            table[0][positionv_kick[0]] = ev_kick[0];  
            e_kick = table[t_kick][position_kick];
            table[t_kick][position_kick] = e;
            //* ****这之间是完成热启动的代码****
            *table[t_kick][position_kick].value = *e_kick.value;
            if(ret_value != NULL)
                *ret_value = *e_kick.value;
            //*
            return false;
        }

        if(positionv_kick[epoch_kick] == position_kick){ // 正好把频率最小的踢出去了
            e_kick = ev_kick[epoch_kick];
            //* ****这之间是完成热启动的代码****
            *table[0][e_position].value = *e_kick.value;
            if(ret_value != NULL)
                *ret_value = *e_kick.value;
            //*
            return false;
        } 
        else{  // 应该把epoch_kick这轮被踢出的元素还原，并把上一轮被踢出的元素插入到table[epoch_kick][position_kick]
            // t_kick等于epoch_kick%2
            table[t_kick][positionv_kick[epoch_kick]] = ev_kick[epoch_kick]; 
            e_kick = table[t_kick][position_kick];
            CuckooEntry e_last; //上一轮被踢出的元素
            if(epoch_kick > 0)
                e_last = ev_kick[epoch_kick-1];
            else // epoch_kick=0时，上一轮被踢出的元素可以视为e
                e_last = e;
            table[t_kick][position_kick] = e_last;
            //* ****这之间是完成热启动的代码****
            *table[t_kick][position_kick].value = *e_kick.value;
            if(ret_value != NULL)
                *ret_value = *e_kick.value;
            //*
            return false;
        }
    }

    double loadFactor(){
        return (double)count_item / (size[0] + size[1]);
    }

// debug functions 
public:
    double print_loadfactor(){
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

    void print_freq_sum(){
        int ctt = 0;
        for(int i = 0; i < 2; ++i)
            for(int j = 0; j < size[i]; ++j)
                ctt += table[i][j].freq;
        printf("freq_sum: %d\n", ctt);
    }
};

#endif
