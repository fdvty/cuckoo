#ifndef _CUCKOO_H_
#define _CUCKOO_H_

#include <algorithm>
#include <cstring>
#include <string>
#include "murmur3.h"
#include <cassert>
#include "immintrin.h"

using namespace std;



inline uint64_t simple_random(uint64_t x){
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

#define KEY_LEN 8
struct CuckooEntry{
    uint64_t key;
    uint64_t* value;
    uint64_t votes;

    CuckooEntry(){
        key = 0;
        value = 0; 
        votes = 0;
    }
    CuckooEntry(uint64_t _key, uint64_t* _value, uint64_t _votes){
        key = _key;
        value = _value;
        votes = _votes;
    }
    CuckooEntry(const CuckooEntry& e){
        key = e.key;
        value = e.value;
        votes = e.votes;
    }
    CuckooEntry& operator=(const CuckooEntry& e){
        key = e.key;
        value = e.value;
        votes = e.votes;
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

#define BUCKET_SIZE 8

struct CuckooBucket{
    uint64_t keys[BUCKET_SIZE];
    uint64_t* values[BUCKET_SIZE];
    uint64_t posVotes[BUCKET_SIZE];
    // uint64_t negVotes; 
    uint64_t used; 

    CuckooBucket(){
        memset(keys, 0, sizeof(keys));
        memset(values, 0, sizeof(values));
        memset(posVotes, 0, sizeof(posVotes));
        // negVotes = 0;
        used = 0;
    }

    inline void bucket_set(const CuckooEntry& e, int i){
        keys[i] = e.key;
        values[i] = e.value;
        posVotes[i] = e.votes;
    }

    inline CuckooEntry bucket_query(int i){
        return CuckooEntry(keys[i], values[i], posVotes[i]);
    }

};


#define MAX_THRESHOLD 100

class Cuckoo{
private:
    CuckooBucket *table[2]; 
    int size[2];            
    int width[2];           
    int threshold;  
    uint32_t count_item;         

    CuckooEntry vec_eKick[MAX_THRESHOLD];
    uint32_t vec_pKick[MAX_THRESHOLD];
    uint32_t vec_iKick[MAX_THRESHOLD];

    int epochKick;
    int tKick;
    int pKick;
    int iKick;
    uint64_t minVotes;

    uint32_t pseed[2];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 25;

        capacity = (capacity + BUCKET_SIZE - 1) / BUCKET_SIZE;
        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
        count_item = 0;

        for(int i = 0; i < 2; ++i){
            table[i] = new CuckooBucket[size[i]];
            memset(table[i], 0, sizeof(CuckooBucket)*size[i]);
        }

        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            pseed[i] = seed;
        }
        assert(threshold < MAX_THRESHOLD);
    }

    ~Cuckoo(){
        delete [] table[0];
    }


private:
    inline int hash_value(uint64_t key, int t){
        return MurmurHash3_x86_32((const char*)&key, KEY_LEN, pseed[t]) % size[t];
    }

    inline uint64_t* query_table(uint64_t key, int t){
        int p = hash_value(key, t);
        for(int i = 0; i < table[t][p].used; ++i){
            if(table[t][p].keys[i] == key){
                table[t][p].posVotes[i]++;
                return table[t][p].values[i];
            }
        }
        return NULL;
    }

    inline bool update_table(uint64_t key, int t, uint64_t delta){
        int p = hash_value(key, t);
        for(int i = 0; i < table[t][p].used; ++i)
            if(table[t][p].keys[i] == key){
                if(delta != 0)
                    *table[t][p].values[i] += delta; 
                return true;
            }
        return false;
    }


    inline bool insert_table(const CuckooEntry &e, int t, int p = -1, int epochNow = 0){
        if(p == -1)
            p = hash_value(e.key, t);
        if(table[t][p].used < BUCKET_SIZE){
            int used = table[t][p].used;
            table[t][p].keys[used] = e.key;
            table[t][p].values[used] = e.value;
            table[t][p].used++;
            count_item++;
            return true;
        }
        for(int i = 0; i < BUCKET_SIZE; ++i){
            if(table[t][p].posVotes[i] < minVotes || (table[t][p].posVotes[i] == minVotes && simple_random(e.key)%2 == 1)){
            // if(table[t][p].posVotes[i] < minVotes){
                minVotes = table[t][p].posVotes[i];
                epochKick = epochNow;
                tKick = t;
                pKick = p;
                iKick = i;
            }
        }
        return false;
    }

public:
    // return NULL if fails
    uint64_t* query(uint64_t key){
        uint64_t* ret = query_table(key, 0);
        if(ret == NULL)
            ret = query_table(key, 1);
        return ret;
    }

    // uint64_t* delta (add *delta to existing value)
    bool SGD_update(const CuckooEntry &e){
        if(update_table(e.key, 0, *e.value) || update_table(e.key, 1, *e.value))
            return true;
        return false;
    }

    // uint64_t* value (store this pointer)
    bool insert(const CuckooEntry &e){
        int p = hash_value(e.key, 0);

        minVotes = 0x3f3f3f3f3f3f3f3f;

        if(insert_table(e, 0, p, 0) || insert_table(e, 1, -1, 0))
            return true;

        CuckooEntry e_insert = e;
        CuckooEntry e_tmp;

        int ep = p;

        for(int k = 0; k < threshold; ++k){
            int t = k%2;
            if(k != 0){
                p = hash_value(e_insert.key, t);
                if(insert_table(e_insert, t, p, k))
                    return true;
            }
            int i = simple_random(e_insert.key)%BUCKET_SIZE;
            e_tmp = table[t][p].bucket_query(i);
            table[t][p].bucket_set(e_insert, i);
            e_insert = e_tmp;

            vec_pKick[k] = p;
            vec_iKick[k] = i;
            vec_eKick[k] = e_tmp;
        }
    

        for(int k = threshold-1; k > epochKick; --k){ 
            int t = k%2; 
            table[t][vec_pKick[k]].bucket_set(vec_eKick[k], vec_iKick[k]);
        }
        CuckooEntry e_kick;
        if(epochKick == 0 && tKick == 1){ 
            table[0][vec_pKick[0]].bucket_set(vec_eKick[0], vec_iKick[0]);
            // e_kick = table[tKick][pKick].bucket_query(iKick);
            table[tKick][pKick].bucket_set(e, iKick);
            return false;
        }

        else if(iKick == vec_iKick[epochKick]){ 
            // e_kick = table[tKick][pKick].bucket_query(iKick);
            return false;
        } 
        else{  // 't_kick' is equal to 'epoch_kick%2'
            table[tKick][vec_pKick[epochKick]].bucket_set(vec_eKick[epochKick], vec_iKick[epochKick]);
            // e_kick = table[tKick][pKick].bucket_query(iKick);
            CuckooEntry e_last;
            if(epochKick > 0)
                e_last = vec_eKick[epochKick-1];
            else 
                e_last = e;
            table[tKick][pKick].bucket_set(e_last, iKick);
            return false;
        }
    }

    double loadFactor(){
        return (double)count_item / ((size[0] + size[1])*BUCKET_SIZE);
    }

// debug functions 
public:
    double print_loadfactor(){
        int full = 0;
        for(int i = 0; i < 2; ++i){
            int tmp = 0;
            for(int j = 0; j < size[i]; ++j)
                tmp += table[i][j].used;
            full += tmp;
            printf("table %d, load factor %lf\n", i, (double)tmp/(size[i]*BUCKET_SIZE));
        }
        assert(full == count_item);
        return (double)full/(size[0] + size[1]);
    }

    uint64_t print_posVoteSum(){
        uint64_t ret = 0;
        for(int i = 0; i < 2; ++i){
            for(int j = 0; j < size[i]; ++j){
                for(int k = 0; k < table[i][j].used; ++k)
                    ret += table[i][j].posVotes[k];
            }
        }
        printf("posVoteSum: %llu\n", ret);
        return ret;
    }
};

#endif
