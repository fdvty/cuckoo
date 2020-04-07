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
//#define MAX_HANG_LEN 



struct Hash_entry;

struct Cuckoo_entry{
    bool flag;
    char key[KEY_LEN];
    char value[VAL_LEN];
    Cuckoo_entry *hang;//the hanging items
    Cuckoo_entry(){
        //flag=false;
        //memset(key,0, KEY_LEN*sizeof(char));
        //memset(value,0,VAL_LEN*sizeof(char));
        hang=NULL;
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
        for(int i = 0; i < 2; ++i)
            delete [] table[i];
    }

    //get the hash of key   
    //@para: t is the s 
    int hash_value(const char* key, int t){
        return bobhash[t].run(key, KEY_LEN*sizeof(char)) % size[t];
    }

    
    bool search_table(const char* key, int t, char* value, int position = -1){
        assert(t == 0 || t == 1);
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
    bool search_the_hang(const char *key, int t=1,char *value = NULL,int position=-1){
        if (position == -1)
            position = hash_value(key, 1);
        
        Cuckoo_entry *current=table[t][position].hang;

        while(current){
            if (current->flag && memcmp(current->key, key, KEY_LEN * sizeof(char)) == 0){
                if (value != NULL)
                    memcpy(value, current->value, VAL_LEN * sizeof(char));
                return true;
            }
            current=current->hang;
        }


        return false;
    }

    bool search(const char *key, char *value = NULL)
    {
        if(search_table(key, 0, value))
            return true;

        int position_tmp=hash_value(key,1);
        if(search_table(key, 1, value,position_tmp)||search_the_hang(key,1,value,position_tmp))
            return true;
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
    //method 0 implies that we just insert to the rear,but once if we have vacancies,we fill it
    //method 1 is from the front
    bool hang_the_item(const Hash_entry &e,int t=1,int position =-1,int method=0){
        if(position==-1)
            position=hash_value(e.key,t);
        assert(method == 0 || method == 1);

        Cuckoo_entry *current = &table[t][position];
        
        if(method==0||current->hang==NULL){
            while(current->hang){
                current=current->hang;
                if(current->flag==false){
                    current->flag = true;
                    memcpy(current->key, e.key, KEY_LEN * sizeof(char));
                    memcpy(current->value, e.value, VAL_LEN * sizeof(char));
                    return true;
                }
                
            }
            
            current->hang = new Cuckoo_entry();
            current=current->hang;
            current->flag = true;
            memcpy(current->key, e.key, KEY_LEN * sizeof(char));
            memcpy(current->value, e.value, VAL_LEN * sizeof(char));    
        }
        else{
            Cuckoo_entry*p_tmp=current->hang;
            current->hang = new Cuckoo_entry();
            current = current->hang;
            current->hang=p_tmp;
            current->flag=true;
            memcpy(current->key, e.key, KEY_LEN * sizeof(char));
            memcpy(current->value, e.value, VAL_LEN * sizeof(char));
            
        }
        
        return true;
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

        
        hang_the_item(e_insert);
        

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

    //newly added
    //method 0 simply switches the flag form positive to negative
    //method 1 frees the space that is useless
    bool remove_the_hang(const Hash_entry&e,int t=1,int position=-1,int method=0){
        if(position==-1){
            position=hash_value(e.key,t);
        }
        assert(method==0||method==1);

        Cuckoo_entry *current = table[t][position].hang;
        Cuckoo_entry *tmp = &table[t][position];
        while (current)
        {
            if (current->flag && memcmp(current->key, e.key, KEY_LEN * sizeof(char)) == 0)
            {
                if(method==0){
                    current->flag = false;
                    return true;
                }
                else{
                    tmp->hang=current->hang;
                    delete current;
                    return true;
                }
            }
            tmp=current;
            current = current->hang;
        }
        return false;
    }
    bool remove(const Hash_entry &e){
        if(remove_table(e, 0))
            return true;
        //newly added
        int position_tmp=hash_value(e.key,1);
        if(remove_table(e,1,position_tmp)||remove_the_hang(e,1,position_tmp,0)){
            return true;
        }
        return false;
    }
};



#endif