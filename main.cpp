#include <iostream>
#include "cuckoo.h"

#include "utils.h"

#define TEST_CUCKOO

const int CUCKOO_SIZE = KV_NUM;

// #define TEST_CUCKOO
#ifdef TEST_CUCKOO
int main(){
    Cuckoo cuckoo(CUCKOO_SIZE);
    create_kv();
    int count_fail = 0;
    int stop = 0;


    bool flag = 0;
    int tmp = 0;
    int FAIL_STOP = 8;

    timespec time1, time2;
    long long resns = 0;

    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int i = 0; i < KV_NUM; ++i){
        if(cuckoo.insert(kv[i]) == false)
            if(++count_fail == FAIL_STOP){
                stop = i;
                break;
            }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
    double insertMips = (double)1000.0 * (stop) / resns; 
    printf("insert stop at %d\n", stop);
    printf("insertion speed: %lf\n", insertMips);
    printf("load factor %lf\n", (double)stop/CUCKOO_SIZE);



    clock_gettime(CLOCK_MONOTONIC, &time1);
    for(int i = 0; i < KV_NUM; ++i){
        if(cuckoo.search(kv[i].key) == false){
            stop = i;
            break;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
    double queryMips = (double)1000.0 * (stop) / resns; 

    printf("search stop at %d\n", stop);
    printf("search speed: %lf\n", queryMips);



    return 0;
}


#else
int main(){
    Cuckoo cuckoo(CUCKOO_SIZE);

    create_kv();
    int count_fail = 0;
    int stop = KV_NUM;

    timespec time1, time2;
    long long resns = 0;

    bool flag = 0;
    int tmp = 0;
    int FAIL_STOP = 200;

    for(int i = 0; i < KV_NUM; ++i){
        if(cuckoo.insert(kv[i]) == false)
            if(++count_fail == FAIL_STOP){
                stop = i;
                break;
            }
        if(!flag && (double)i/CUCKOO_SIZE >= 0.9){
            flag = true;
            tmp = i;
            clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double insertMips = (double)1000.0 * (stop - tmp) / resns; // calculate time

    double loadFactor = (double)(stop - FAIL_STOP)/CUCKOO_SIZE;


    resns = 0;
    int cntSearchFail = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
    for(int i = 0; i <= stop; ++i){
        if(cuckoo.search(kv[i].key) == false)
            cntSearchFail++;
    }
    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double queryMips = (double)1000.0 * stop / resns; // calculate time

    // assert(cntSearchFail == FAIL_STOP);
    printf("\n-----------------------result-----------------------\n");
    printf("hash table size: %d, inserted item: %d\n", CUCKOO_SIZE, stop);
    printf("load factor: %lf, insert MIPS: %lf, query MIPS: %lf\n", loadFactor, insertMips, queryMips);

    printf("\n-----------------------debug-----------------------\n");
    printf("stop: %d, cntSearchFail: %d\n", stop, cntSearchFail);
    printf("max link: %d, total link: %d\n", cuckoo.max_link_length(), cuckoo.total_link_length());

    return 0;
}
#endif