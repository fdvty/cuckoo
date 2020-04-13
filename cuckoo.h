#ifndef CUCKOO_H
#define CUCKOO_H

#include <algorithm>
#include <cstring>
#include <string>
#include "BOBHash32.h"
#include <cassert>

using namespace std;



#define KEY_LEN 20
#define VAL_LEN 10

struct Hash_entry;

struct Cuckoo_entry{
    bool flag;
    char key[KEY_LEN];
    char value[VAL_LEN];
    Cuckoo_entry *hang; //the hanging items

    Cuckoo_entry(){
        flag = false;
        hang = NULL;
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
    Cuckoo_entry *table[2]; //
    int size[2];            //
    int width[2];           //how many in this one
    int threshold;          //maybe the kickout threshold?

    BOBHash32 bobhash[2];   //

public:
    Cuckoo(int capacity){
        width[0] = 9;
        width[1] = 3;
        threshold = 30;

        size[0] = capacity * width[0] / (width[0] + width[1]);  //
        size[1] = capacity - size[0];                           //
        
        //initializing and apportion
        for(int i = 0; i < 2; ++i){
            table[i] = new Cuckoo_entry[size[i]];
            for(int j = 0; j < size[i]; ++j)
                table[i][j].flag = 0;
        }

        //
        for(int i = 0; i < 2; ++i){
            uint32_t seed = rand()%MAX_PRIME32;
            bobhash[i].initialize(seed);
        }
    }

    ~Cuckoo(){
        for(int i = 0; i < size[1]; ++i){
            Cuckoo_entry* tmp = table[1][i].hang;
            Cuckoo_entry* next;
            while(tmp){
                next = tmp->hang;
                delete tmp;
                tmp = next;
            }
        }
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }


private:
    int hash_value(const char* key, int t){
        return bobhash[t].run(key, KEY_LEN*sizeof(char)) % size[t];
    }

    bool search_table(const char* key, int t, char* value, int position = -1){
        if(position == -1)
            position = hash_value(key, t);
        for(int i = position; i < position + width[t]; ++i)
            //get the wanted item
            if(table[t][i].flag && memcmp(table[t][i].key, key, KEY_LEN*sizeof(char)) == 0){
                if(value != NULL)
                    memcpy(value, table[t][i].value, VAL_LEN*sizeof(char));
                return true;
            }
        
        return false;
    }

    //newly added
    bool search_list(const char *key, int t=1,char *value = NULL,int position = -1){
        if (position == -1)
            position = hash_value(key, 1);
        
        Cuckoo_entry *current = table[t][position].hang;

        while(current){
            // assert(current->flag);
            if (memcmp(current->key, key, KEY_LEN * sizeof(char)) == 0){
                if (value != NULL)
                    memcpy(value, current->value, VAL_LEN * sizeof(char));
                return true;
            }
            current = current->hang;
        }
        return false;
    }

    bool insert_table(const Hash_entry &e, int t, int position = -1){
        if(position == -1)
            position = hash_value(e.key, t);//compute the address

        for(int i = position; i < position + width[t]; ++i)
            if(!table[t][i].flag){
                table[t][i].flag = true;
                memcpy(table[t][i].key, e.key, KEY_LEN*sizeof(char));
                memcpy(table[t][i].value, e.value, VAL_LEN*sizeof(char));
                return true;
            }

        return false;
    }

    //newly added
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

    //newly added
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

public:
    bool search(const char *key, char *value = NULL){
        if(search_table(key, 0, value))
            return true;

        int position_tmp = hash_value(key,1);
        if(search_table(key, 1, value, position_tmp) || search_list(key, 1, value, position_tmp))
            return true;
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
        hang_the_item(e_insert, 1);
        return false;
    }

    bool remove(const char* key){
        if(remove_table(key, 0))
            return true;
        //newly added
        int position_tmp = hash_value(key, 1);
        if(remove_table(key, 1, position_tmp) || remove_list(key, 1, position_tmp))
            return true;
        return false;
    }

// below are debug functions
public:
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

};



#endif