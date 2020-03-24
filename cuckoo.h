#ifndef CUCKOO_H
#define CUCKOO_H

#include <algorithm>
#include <cstring>
#include <string>
#include "BOBHash32.h"

using namespace std;


#define KEY_LEN 20
#define VAL_LEN 10

struct Hash_entry;

struct Cuckoo_entry{
    bool flag;
    char key[KEY_LEN];
    char value[VAL_LEN];

    Cuckoo_entry& operator=(const Hash_entry& e);
};

struct Hash_entry{
    char key[KEY_LEN];
    char value[VAL_LEN];

    Hash_entry(){}
    Hash_entry(const Cuckoo_entry& e){
        memcpy(key, e.key, KEY_LEN*sizeof(char));
        memcpy(value, e.value, VAL_LEN*sizeof(char));
    }
    Hash_entry& operator=(const Cuckoo_entry& e){
        memcpy(key, e.key, KEY_LEN*sizeof(char));
        memcpy(value, e.value, VAL_LEN*sizeof(char));
        return *this;
    }
};

Cuckoo_entry& Cuckoo_entry::operator=(const Hash_entry& e){
    memcpy(key, e.key, KEY_LEN*sizeof(char));
    memcpy(value, e.value, VAL_LEN*sizeof(char));
    // flag = true;
    return *this;
}


class Cuckoo{
private:
    Cuckoo_entry *table[2];
    int size[2];
    int width[2];
    int threshold;

    BOBHash32 bobhash[2];

public:
    Cuckoo(int capacity){
        width[0] = 4;
        width[1] = 4;
        threshold = 35;

        size[0] = capacity * width[0] / (width[0] + width[1]);
        size[1] = capacity - size[0];
        // size[0] = capacity/2;
        // size[1] = capacity - size[0];
        
        for(int i = 0; i < 2; ++i){
            table[i] = new Cuckoo_entry[size[i]];
            for(int j = 0; j < size[i]; ++j)
                table[i][j].flag = 0;
        }

        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            bobhash[i].initialize(seed);
        }
    }

    ~Cuckoo(){
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }

    int hash_value(const char* key, int t){
        return bobhash[t].run(key, KEY_LEN*sizeof(char)) % size[t];
    }

    bool search_table(const char* key, int t, char* value, int position = -1){
        assert(t == 0 || t == 1);
        if(position == -1)
            position = hash_value(key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].flag && memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                if(value != NULL)
                    memcpy(value, table[t][i].value, VAL_LEN*sizeof(char));
                return true;
            }
        return false;
    }

    bool search(const char* key, char* value = NULL){
        if(search_table(key, 0, value) || search_table(key, 1, value))
            return true;
        return false;
    }

    bool insert_table(const Hash_entry &e, int t, int position = -1){
        if(position == -1)
            position = hash_value(e.key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(!table[t][i].flag){
                table[t][i].flag = true;
                memcpy(table[t][i].key, e.key, KEY_LEN*sizeof(char));
                memcpy(table[t][i].value, e.value, VAL_LEN*sizeof(char));
                return true;
            }
        return false;
    }

    bool insert(const Hash_entry &e){
        if(insert_table(e, 0) || insert_table(e, 1))
            return true;
        int h[2];
        Hash_entry e_insert = e;
        Hash_entry e_tmp;
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

    bool remove_table(const Hash_entry &e, int t, int position = -1){
        if(position == -1)
            position = hash_value(e.key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].flag && memcmp(table[t][i].key, e.key, KEY_LEN*sizeof(char)) == 0){
                table[t][i].flag = false;
                return true;
            }
        return false;
    }

    bool remove(const Hash_entry &e){
        if(remove_table(e, 0) || remove_table(e, 1))
            return true;
        return false;
    }
};



#endif