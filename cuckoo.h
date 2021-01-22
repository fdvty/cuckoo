#ifndef _CUCKOO_H_
#define _CUCKOO_H_

#include <algorithm>
#include <cstring>
#include <string>
#include <cmath>
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

inline void set_flag(uint64_t& x, int t){
    x |= (1 << t);
}

inline int query_flag(uint64_t x){
    for(int i = 0; i < 64; ++i){
        if((x&1) == 1)
            return i;
        x >>= 1;
    }
    return -1;
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

#define BUCKET_SIZE 4

struct CuckooBucket{
    uint64_t keys[BUCKET_SIZE];
    uint64_t* values[BUCKET_SIZE];
    uint64_t posVotes[BUCKET_SIZE];
    uint64_t negVotes; 
    uint64_t used; 
    uint64_t kingFlag;

    CuckooBucket(){
        memset(keys, 0, sizeof(keys));
        memset(values, 0, sizeof(values));
        memset(posVotes, 0, sizeof(posVotes));
        negVotes = 0;
        used = 0;
        kingFlag = 0;
    }

    inline void bucket_set(const CuckooEntry& e, int i, bool clearNeg = false){
        keys[i] = e.key;
        values[i] = e.value;
        posVotes[i] = e.votes;
        if(clearNeg)
            negVotes = 0;
    }

    inline CuckooEntry bucket_query(int i){
        return CuckooEntry(keys[i], values[i], posVotes[i]);
    }

};


#define MAX_THRESHOLD 100

#define KING_THRESHOLD 10000

class Cuckoo{
private:
    CuckooBucket *table;                   
    int threshold;  
    uint32_t count_item;   
    int capacity;      

    CuckooEntry vec_eKick[MAX_THRESHOLD];
    uint32_t vec_pKick[MAX_THRESHOLD];
    uint32_t vec_iKick[MAX_THRESHOLD];

    uint32_t pseed[2];

public:
    Cuckoo(int size){
        threshold = 25;
        capacity = (size + BUCKET_SIZE - 1) / BUCKET_SIZE;                      
        count_item = 0;

        table = new CuckooBucket[capacity];
        memset(table, 0, sizeof(CuckooBucket)*capacity);

        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            pseed[i] = seed;
        }
        assert(threshold < MAX_THRESHOLD);
    }

    ~Cuckoo(){
        delete [] table;
    }


private:
    inline int hash_value(uint64_t key, int t){
        return MurmurHash3_x86_32((const char*)&key, KEY_LEN, pseed[t]) % capacity;
    }

    inline uint64_t* query_table(uint64_t key, int t){
        int p = hash_value(key, t);
        for(int i = 0; i < table[p].used; ++i){
            if(table[p].keys[i] == key){
                table[p].posVotes[i]++;
                if(table[p].kingFlag == 0 && table[p].posVotes[i] > KING_THRESHOLD)
                    set_flag(table[p].kingFlag, i);
                return table[p].values[i];
            }
        }
        table[p].negVotes++;
        return NULL;
    }

    inline bool update_table(uint64_t key, int t, uint64_t delta){
        int p = hash_value(key, t);
        for(int i = 0; i < table[p].used; ++i)
            if(table[p].keys[i] == key){
                if(delta != 0)
                    *table[p].values[i] += delta; 
                return true;
            }
        return false;
    }


    inline bool insert_table(const CuckooEntry &e, int t, int p = -1){
        if(p == -1)
            p = hash_value(e.key, t);
        if(table[p].used < BUCKET_SIZE){
            int used = table[p].used;
            table[p].bucket_set(e, used, true); // or only true if: epochNow==0
            table[p].used++;
            count_item++;
            return true;
        }
        return false;
    }



public:
    // return NULL if fails
    uint64_t* cuckoo_query(uint64_t key){
        uint64_t* ret = query_table(key, 0);
        if(ret == NULL)
            ret = query_table(key, 1);
        return ret;
    }

    // uint64_t* delta (add *delta to existing value)
    bool cuckoo_update(const CuckooEntry &e){
        if(update_table(e.key, 0, *e.value) || update_table(e.key, 1, *e.value))
            return true;
        return false;
    }

    // uint64_t* value (store this pointer)
    bool cuckoo_insert(CuckooEntry e){
        int p = hash_value(e.key, 0);

        if(insert_table(e, 0, p) || insert_table(e, 1, -1))
            return true;

        CuckooEntry e_insert = e;
        CuckooEntry e_tmp;

        for(int k = 0; k < threshold; ++k){
            int t = k%2;
            if(k != 0){
                p = hash_value(e_insert.key, t);
                if(insert_table(e_insert, t, p))
                    return true;
            }
            int i = simple_random(e_insert.key)%BUCKET_SIZE;
            // int i = rand()%BUCKET_SIZE;
            e_tmp = table[p].bucket_query(i);
            table[p].bucket_set(e_insert, i);
            e_insert = e_tmp;

            vec_pKick[k] = p;
            vec_iKick[k] = i;
            vec_eKick[k] = e_tmp;
        }

        for(int k = threshold-1; k >= 0; --k){ 
            int t = k%2; 
            table[vec_pKick[k]].bucket_set(vec_eKick[k], vec_iKick[k]);
        }

        return false;
    }

    bool elastic_insert(CuckooEntry e, bool cuckoo_fail = false){
        int p = hash_value(e.key, 0);
        
        if(!cuckoo_fail)
            if(insert_table(e, 0, p) || insert_table(e, 1, -1))
                return true;
        
        int minPosVotes = 0x3f3f3f3f;
        int minPos = -1;
        for(int i = 0; i < BUCKET_SIZE; ++i){
            if(table[p].posVotes[i] < minPosVotes && (((table[p].kingFlag >> i)&1) == 0) ){
                minPosVotes = table[p].posVotes[i];
                minPos = i;
            }
        }
        if(table[p].negVotes > minPosVotes){
            e.votes = 0;
            table[p].bucket_set(e, minPos, true);
            return true;
        }
        return false;
    }

    uint64_t* elastic_query(CuckooEntry e){
        int p = hash_value(e.key, 0);
        if(table[p].kingFlag == 0)
            return NULL;
        int pos = query_flag(table[p].kingFlag);
        return table[p].values[pos];
    }

    bool elastic_update(uint64_t key, uint64_t delta){
        int p = hash_value(key, 0);
        int pos = query_flag(table[p].kingFlag);
        *table[p].values[pos] += delta;
        return true;
    }


    double loadFactor(){
        return (double)count_item / (capacity *BUCKET_SIZE);
    }

// debug functions 
public:
    double print_loadfactor(){
        int full = 0;
        for(int j = 0; j < capacity; ++j)
            full += table[j].used;
        assert(full == count_item);
        double lf = (double)count_item/(capacity*BUCKET_SIZE); 
        printf("load factor %lf\n", lf);
        return lf;
    }

    uint64_t print_posVoteSum(){
        uint64_t ret = 0;
        for(int j = 0; j < capacity; ++j){
            for(int k = 0; k < table[j].used; ++k)
                ret += table[j].posVotes[k];
        }
        printf("posVoteSum: %llu\n", ret);
        return ret;
    }
};

#endif
