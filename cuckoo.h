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

struct Hash_entry;

struct Cuckoo_entry{
    char key[KEY_LEN];
    char value[VAL_LEN];
    bool flag;

#ifdef _HANG_LINK
    Cuckoo_entry *hang; //the hanging items
#endif 

    Cuckoo_entry(){
        flag = false;
#ifdef _HANG_LINK
        hang = NULL;
#endif
    }
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

    uint32_t hashseed[2];

public:
    Cuckoo(int capacity){
        width[0] = 8;
        width[1] = 8;
        threshold = 15;

        size[0] = capacity * 3 / (3 + 1);  
        size[1] = capacity - size[0];                           
        
        // initialize two sub-tables
        for(int i = 0; i < 2; ++i){
            table[i] = new Cuckoo_entry[size[i]];
            memset(table[i], 0, sizeof(Cuckoo_entry)*size[i]);
        }
        // initialize hash funcitons
        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            hashseed[i] = seed;
        }
    }

    ~Cuckoo(){
#ifdef _HANG_LINK
        for(int i = 0; i < size[1]; ++i){
            Cuckoo_entry* tmp = table[1][i].hang;
            Cuckoo_entry* next;
            while(tmp){
                next = tmp->hang;
                delete tmp;
                tmp = next;
            }
        }
#endif
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }


private:
    int hash_value(const char* key, int t){
        return MurmurHash3_x86_32(key, KEY_LEN, hashseed[t]) % (size[t] - width[t] + 1);
    }

    bool search_table(const char* key, int t, char* value = NULL, int position = -1, const char* cover_value = NULL){
        if(position == -1)
            position = hash_value(key, t);

        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].flag && memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                if(cover_value != NULL)
                    memcpy(table[t][i].value, cover_value, VAL_LEN*sizeof(char));
                if(value != NULL)
                    memcpy(value, table[t][i].value, VAL_LEN*sizeof(char));
                return true;
            }
        
        return false;
    }

#ifdef _HANG_LINK
    bool search_list(const char *key, int t=1,char *value = NULL,int position = -1, const char* cover_value = NULL){
        if (position == -1)
            position = hash_value(key, 1);
        
        Cuckoo_entry *current = table[t][position].hang;

        while(current){
            // assert(current->flag);
            if (memcmp(current->key, key, KEY_LEN * sizeof(char)) == 0){
                if(cover_value != NULL)
                    memcpy(current->value, cover_value, VAL_LEN *sizeof(char));
                if(value != NULL)
                    memcpy(value, current->value, VAL_LEN * sizeof(char));
                return true;
            }
            current = current->hang;
        }
        return false;
    }
#endif

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

#ifdef _HANG_LINK
    void hang_the_item(const Hash_entry &e, int t = 1, int position = -1){
        if(position == -1)
            position = hash_value(e.key, t);
        
        Cuckoo_entry *next = table[t][position].hang;
        table[t][position].hang = new Cuckoo_entry();
        Cuckoo_entry *current = table[t][position].hang;
        current->flag = true;
        current->hang = next;
        memcpy(current->key, e.key, KEY_LEN * sizeof(char));
        memcpy(current->value, e.value, VAL_LEN * sizeof(char));      
    }
#endif
    
    bool remove_table(const char *key, int t, int position = -1){
        if(position == -1)
            position = hash_value(key, t);
        for(int i = position; i < position + width[t]; ++i)
            if(table[t][i].flag && memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                table[t][i].flag = false;
                return true;
            }
        return false;
    }

#ifdef _HANG_LINK
    bool remove_list(const char* key, int t=1, int position=-1){
        if(position==-1)
            position=hash_value(key,t);

        Cuckoo_entry *current = table[t][position].hang;
        Cuckoo_entry *prev = &table[t][position];
        while (current){
            // assert(prev->hang == current);
            // assert(current->flag);
            if (memcmp(current->key, key, KEY_LEN * sizeof(char)) == 0){
                prev->hang = current->hang;
                delete current;
                return true;
            }
            prev = current;
            current = current->hang;
        }
        return false;
    }
#endif

public:
    bool search(const char *key, char *value = NULL, const char *cover_value = NULL){
        if(search_table(key, 0, value, -1, cover_value))
            return true;
        int position_tmp = hash_value(key,1);
        if(search_table(key, 1, value, position_tmp, cover_value))
            return true;
#ifdef _HANG_LINK
        if(search_list(key, 1, value, position_tmp, cover_value))
            return true;
#endif
        return false;
    }

    bool insert(const Hash_entry &e){
        if(search(e.key, NULL, e.value))
            return true;

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
#ifdef _HANG_LINK 
        hang_the_item(e_insert, 1);
#endif
        return false;
    }

    bool remove(const char* key){
        if(remove_table(key, 0))
            return true;
        int position_tmp = hash_value(key, 1);
        if(remove_table(key, 1, position_tmp))
            return true;
#ifdef _HANG_LINK
        if(remove_list(key, 1, position_tmp))
            return true;
#endif
        return false;
    }

public:
    double loadfactor(){
        int full = 0;
        for(int i = 0; i < 2; ++i){
            int tmp = 0;
            for(int j = 0; j < size[i]; ++j)
                tmp += table[i][j].flag;
            full += tmp;
            printf("table %d, load factor %lf\n", i, (double)tmp/size[i]);
        }
        return (double)full/(size[0] + size[1]);
    }

#ifdef _HANG_LINK
// below are debug functions
    int max_link_length(){
        int ret = 0;
        for(int i = 0; i < size[1]; ++i){
            int len = 0;
            Cuckoo_entry *current = table[1][i].hang;
            while(current){
                len++;
                current = current->hang;
            }
            if(len > ret) ret = len;
        }
        return ret;
    }
    int total_link_length(){
        int ret = 0;
        for(int i = 0; i < size[1]; ++i){
            int len = 0;
            Cuckoo_entry *current = table[1][i].hang;
            while(current){
                len++;
                current = current->hang;
            }
            ret += len;
        }
        return ret;
    }
#endif

};

#endif
