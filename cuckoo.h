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
    uint64_t key;
    uint64_t* value;

    CuckooEntry(){
        key = 0;
        value = 0;    
    }
    CuckooEntry(const CuckooEntry& e){
        key = e.key;
        value = e.value;
    }
    CuckooEntry& operator=(const CuckooEntry& e){
        key = e.key;
        value = e.value;
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

    int h[2];
    CuckooEntry e_kick[MAX_THRESHOLD];
    uint32_t hash_kick[MAX_THRESHOLD];

    uint32_t hashseed[2];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 35;

        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
        count_item = 0;

        for(int i = 0; i < 2; ++i){
            table[i] = new CuckooEntry[size[i]];
            memset(table[i], 0, sizeof(CuckooEntry)*size[i]);
        }

        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            hashseed[i] = seed;
        }
        assert(threshold < MAX_THRESHOLD);
    }

    ~Cuckoo(){
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }


private:
    inline int hash_value(uint64_t key, int t){
        return MurmurHash3_x86_32((const char*)&key, KEY_LEN, hashseed[t]) % (size[t] - width[t] + 1);
    }

    inline bool query_table(uint64_t key, int t, uint64_t* ret_value = NULL, uint64_t delta = 0){
        if(h[t] == -1)
            h[t] = hash_value(key, t);
        int position = h[t];

        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].key == key){
                if(delta != 0){
                    *table[t][i].value += delta; 
                }
                if(ret_value != NULL)
                    *ret_value = *table[t][i].value;
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
                table[t][i] = e;
                count_item++; 
                return true;
            }

        return false;
    }

public:
    bool query(uint64_t key, uint64_t* ret_value = NULL){
        h[0] = h[1] = -1;
        if(query_table(key, 0, ret_value, 0) || query_table(key, 1, ret_value, 0))
            return true;
        return false;
    }

    // <uint64_t key, uint64_t* delta> 
    bool insert_old(const CuckooEntry &e){
        h[0] = h[1] = -1;
        if(query_table(e.key, 0, NULL, *e.value) || query_table(e.key, 1, NULL, *e.value))
            return true;
        return false;
    }

    // <uint64_t key, uint64_t* value> 
    bool insert_new(const CuckooEntry &e){
        h[0] = h[1] = -1;
        
        if(insert_table(e, 0) || insert_table(e, 1))
            return true;

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

            hash_kick[k] = h[t];
            e_kick[k] = e_tmp;
        }

        for(int k = threshold-1; k >= 0; --k){
            int t = k%2; 
            table[t][hash_kick[k]] = e_kick[k];
        }

        return false;
    }

// debug functions 
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
