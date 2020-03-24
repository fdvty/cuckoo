#include <iostream>
#include "cuckoo.h"

#include "utils.h"


void test(){
    Hash_entry e;
    // memcpy(es.key, "1234567890123456789", KEY_LEN*sizeof(char));
    memcpy(e.key, "1234567890123456789", KEY_LEN*sizeof(char));
    memcpy(e.value, "123456789", VAL_LEN*sizeof(char));

    Cuckoo_entry ce;
    ce.flag = false;
    ce = e;
    printf("%s\n%s\n%d\n", ce.key, ce.value, ce.flag);
    // cuckoo.insert_table(e, 0);
    // if(cuckoo.search_table(es.key, 0, es.value))
    //     printf("%s\n", es.value);

    // printf("%s\n", cuckoo.table[0][12].key);

    Cuckoo_entry ec;
    memcpy(ec.key, "qwertyuiopqwertyuio", KEY_LEN*sizeof(char));
    memcpy(ec.value, "asdfghjkl", VAL_LEN*sizeof(char));
    ec.flag = 1;
    Hash_entry es = ec;
    printf("%s\n%s\n", es.key, es.value);


    // Hash_entry et = ec;
    // printf("%s\n%s\n", et.key, et.value);
    // memcpy(ec.key, "1wertyuiopqwertyuio", KEY_LEN*sizeof(char));
    // et = ec;
    // printf("%s\n", et.key);
}

int main(){
    Cuckoo cuckoo(10000);

    create_kv();
    int count_fail = 0;
    int stop = 0;

    for(int i = 0; i < KV_NUM; ++i)
        if(cuckoo.insert(kv[i]) == false)
            if(++count_fail == 10){
                stop = i;
                break;
            }
    printf("%d\n", stop);


    // for(int i = 0; i < KV_NUM; ++i){
    //     if(cuckoo.insert(kv[i]) == false){
    //         printf("stop at %d\n", i);
    //         stop = i;
    //         break;
    //     }
    // }
    // for(int i = stop+1; i >= stop-10; --i){
    //     if(cuckoo.remove(kv[i]) == false)
    //         printf("remove failed at %d\n", i);
    // }
    // for(int i = 0; i <= stop; ++i){
    //     if(cuckoo.search(kv[i].key) == false)
    //         printf("search failed at %d\n", i);
    // }

    return 0;
}