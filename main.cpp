#include <iostream>
#include "cuckoo.h"
#include "utils.h"


const int CUCKOO_SIZE = KV_NUM/10;


// int main(){
//     Cuckoo cuckoo(CUCKOO_SIZE);


//     create_kv();
//     int count_fail = 0;
//     int stop = KV_NUM-1;

//     timespec time1, time2;
//     long long resns = 0;

//     bool flag = 0;
//     int tmp = 0;
//     int FAIL_STOP = CUCKOO_SIZE*0.00005;
//     FAIL_STOP = CUCKOO_SIZE*9;
//     printf("fail stop: %d\n", FAIL_STOP);

//     for(int i = 0; i < KV_NUM; ++i){
//         if(cuckoo.insert(kv[i]) == false){
//             // cuckoo.enforce_update(kv[i]);
//             if(++count_fail == FAIL_STOP){
//                 stop = i;
//                 break;
//             }
//         }
//         if(!flag && (double)i/CUCKOO_SIZE >= 0.9){
//             flag = true;
//             tmp = i;
//             clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
//         }
//     }

//     printf("stop: %d\n", stop);

//     clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
//     resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
//     double insertMips = (double)1000.0 * (stop - tmp) / resns; // calculate time


//     resns = 0;
//     int cntSearchFail = 0;
//     clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
//     for(int i = 0; i <= stop; ++i){
//         if(cuckoo.query(kv[i].key) == false){
//             cntSearchFail++;
//             printf("fail id: %d\n", i);
//         }
//     }
//     clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
//     resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
//     double queryMips = (double)1000.0 * stop / resns; // calculate time

//     printf("-----------------------result-----------------------\n");
//     printf("hash table size: %d, inserted item: %d\n", CUCKOO_SIZE, stop+1);
//     printf("load factor: %lf, insert speed: %lfM/s, query speed: %lfM/s\n", cuckoo.print_loadfactor(), insertMips, queryMips);

//     printf("-----------------------debug-----------------------\n");
//     printf("stop: %d, cntSearchFail: %d\n", stop, cntSearchFail);




//     delete_kv();
//     return 0;
// }


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
    FAIL_STOP = 1;
    printf("fail stop: %d\n", FAIL_STOP);

    for(int i = 0; i < CUCKOO_SIZE; ++i){
        if(cuckoo.insert(kv[i]) == false){
            // cuckoo.enforce_update(kv[i]);
            if(++count_fail==FAIL_STOP){
                stop=i;
                break;
            }
        }
        if(!flag && (double)i/CUCKOO_SIZE >= 0.9){
            flag = true;
            tmp = i;
            clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
        }
        // stop = i;
    }

    printf("stop: %d\n", stop);

    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double insertMips = (double)1000.0 * (stop - tmp) / resns; // calculate time

    cuckoo.print_freq_sum();

    resns = 0;
    int cntSearchFail = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
    for(int i = 0; i <= CUCKOO_SIZE/2+CUCKOO_SIZE/2; ++i){
        uint64_t ret;
        if(cuckoo.query(kv[i].key, &ret) == false){
            cntSearchFail++;
            // printf("fail id: %d\n", i);
        }
        if(i%100000==0||i==CUCKOO_SIZE-1){
            printf("** %llu\n",ret);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double queryMips = (double)1000.0 * (CUCKOO_SIZE/2*2) / resns; // calculate time

    cuckoo.print_freq_sum();

    printf("-----------------------result-----------------------\n");
    printf("hash table size: %d\n", CUCKOO_SIZE);
    printf("load factor: %lf, insert speed: %lfM/s, query speed: %lfM/s\n", cuckoo.print_loadfactor(), insertMips, queryMips);

    printf("-----------------------debug-----------------------\n");
    printf("stop: %d, cntSearchFail: %d\n", stop, cntSearchFail);

    int cntp = 0, cntf=0;
    for(int i = stop+1;i<stop+CUCKOO_SIZE/20; ++i){
        uint64_t ret;
        if(cuckoo.insert(kv[i], &ret) == false){
            cntf++;
            if(ret > stop)
                cntp++;
            if(i%100000 == 0)
                printf("~~ %llu\n",ret);
        }
    }
    printf("cntf: %d, cntp: %d\n", cntf, cntp);
    cuckoo.print_loadfactor();

    // printf("debug:%d\n",cuckoo.debug);
    delete_kv();
    return 0;
}