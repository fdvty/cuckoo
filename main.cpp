#include <iostream>
#include "cuckoo.h"
#include "utils.h"


const int CUCKOO_SIZE = KV_NUM;


int main(){
    Cuckoo cuckoo(CUCKOO_SIZE);

    create_kv();
    int count_fail = 0;
    int stop = KV_NUM-1;

    timespec time1, time2;
    long long resns = 0;

    bool flag = 0;
    int tmp = 0;
    int FAIL_STOP = CUCKOO_SIZE*0.00005;
    printf("fail stop: %d\n", FAIL_STOP);

    for(int i = 0; i < KV_NUM; ++i){
        if(cuckoo.insert(kv[i]) == false)
            if(++count_fail == FAIL_STOP){
                stop = i;
                break;
            }
        if(!flag && (double)i/CUCKOO_SIZE >= 0.0){
            flag = true;
            tmp = i;
            clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
        }
    }


    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double insertMips = (double)1000.0 * (stop - tmp) / resns; // calculate time

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

    printf("-----------------------result-----------------------\n");
    printf("hash table size: %d, inserted item: %d\n", CUCKOO_SIZE, stop+1);
    printf("load factor: %lf, insert MIPS: %lf, query MIPS: %lf\n", cuckoo.loadfactor(), insertMips, queryMips);

    printf("-----------------------debug-----------------------\n");
    printf("stop: %d, cntSearchFail: %d\n", stop, cntSearchFail);
    return 0;
}
