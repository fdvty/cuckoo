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


class Cuckoo{
private:
    CuckooEntry *table[2]; 
    int size[2];            
    int width[2];           
    int threshold;          

    uint32_t hashseed[2];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 35;

        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
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
    }

    ~Cuckoo(){
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }


private:
    int hash_value(const char* key, int t){
        return MurmurHash3_x86_32(key, KEY_LEN, hashseed[t]) % (size[t] - width[t] + 1);
    }

    bool search_table(const char* key, int t, char* value = NULL, int position = -1, const char* cover_value = NULL, bool remove_flag = false){
        if(position == -1)
            position = hash_value(key, t);

        for(int i = position; i < position + width[t]; ++i)
            if(memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                if(remove_flag)
                    table[t][i] = empty_entry;
                else{
                    if(cover_value != NULL)
                        memcpy(table[t][i].value, cover_value, VAL_LEN*sizeof(char));
                    if(value != NULL)
                        memcpy(value, table[t][i].value, VAL_LEN*sizeof(char));
                }
                return true;
            }
        
        return false;
    }


    bool insert_table(const CuckooEntry &e, int t, int position = -1){
        if(position == -1)
            position = hash_value(e.key, t);

        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i] == empty_entry){
                memcpy(table[t][i].key, e.key, KEY_LEN*sizeof(char));
                memcpy(table[t][i].value, e.value, VAL_LEN*sizeof(char));
                return true;
            }

        return false;
    }

public:
    bool search(const char *key, char *value = NULL, const char *cover_value = NULL){
        if(search_table(key, 0, value, -1, cover_value) || search_table(key, 1, value, -1, cover_value))
            return true;
        return false;
    }

    bool insert(const CuckooEntry &e){
        if(search(e.key, NULL, e.value))
            return true;

        if(insert_table(e, 0) || insert_table(e, 1))
            return true;
        int h[2];
        CuckooEntry e_insert = e;
        CuckooEntry e_tmp;

        for(int k = 0; k < threshold; ++k){
            int t = k%2;
            h[t] = hash_value(e_insert.key, t);
            if(insert_table(e_insert, t, h[t]))
                return true;
            e_tmp = table[t][h[t]];
            table[t][h[t]] = e_insert;
            e_insert = e_tmp;
        }
        return false;
    }

    bool remove(const char* key){
        if(search_table(key, 0, NULL, -1, NULL, true) || search_table(key, 1, NULL, -1, NULL, true))
            return true;
        return false;
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
        return (double)full/(size[0] + size[1]);
    }
};

#endif
