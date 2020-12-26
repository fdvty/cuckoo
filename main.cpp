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
    int FAIL_STOP = CUCKOO_SIZE*0.005;
    // FAIL_STOP = 1;
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
        stop = i;
    }

    printf("stop: %d\n", stop);

    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double insertMips = (double)1000.0 * (stop - tmp) / resns; // calculate time


    resns = 0;
    int cntSearchFail = 0;
    clock_gettime(CLOCK_MONOTONIC, &time1); // calculate time
    for(int i = 0; i <= stop; ++i){
        uint64_t* ret = cuckoo.query(kv[i].key);
        if(ret == NULL){
            cntSearchFail++;
            // printf("fail id: %d\n", i);
        }
        if(i%100000==0){
            if(ret != NULL)
                printf("** %llu\n", *ret);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &time2); // calculate time
    resns += (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); // calculate time
    double queryMips = (double)1000.0 * (CUCKOO_SIZE/2*2) / resns; // calculate time

    cuckoo.print_posVoteSum();

    printf("-----------------------result-----------------------\n");
    printf("hash table size: %d\n", CUCKOO_SIZE);
    printf("load factor: %lf, insert speed: %lfM/s, query speed: %lfM/s\n", cuckoo.print_loadfactor(), insertMips, queryMips);

    printf("-----------------------debug-----------------------\n");
    printf("stop: %d, cntSearchFail: %d\n", stop, cntSearchFail);

    int avoid_o2 = 0;
    // for(int iter = 0; iter < 5; ++iter){
    //     for(int i = CUCKOO_SIZE/2; i <= CUCKOO_SIZE; i++){
    //         if(cuckoo.query(kv[i].key) == NULL)
    //             avoid_o2++;
    //     }
    // }


    cuckoo.print_posVoteSum();
    for(int i = stop+1; i <= CUCKOO_SIZE*3; ++i)
        cuckoo.insert(kv[i]);

    if(cuckoo.query(kv[CUCKOO_SIZE*3].key) == NULL){
        printf("kuku\n");
    }
    else printf("haha\n");

    printf("de: %llu\n", *kv[CUCKOO_SIZE*3].value);

    int fails = 0;
    for(int i = 0; i < CUCKOO_SIZE; ++i){
        if(cuckoo.query(kv[i].key) == NULL)
            fails++;
    }

    printf("fails: %d, total: %d\n", fails, CUCKOO_SIZE/2);
    cuckoo.print_loadfactor();
    printf("ignore: %d\n", avoid_o2);

    int ttp[10] = {};
    for(int i = 0; i <= CUCKOO_SIZE*3; ++i){
        uint64_t* ret = cuckoo.query(kv[i].key);
        if(ret != NULL)
            ttp[(*ret)/(CUCKOO_SIZE/2)]++;
    }
    int sum = 0;
    for(int i = 0; i < 9; ++i){
        printf("%d ", ttp[i]);
        sum += ttp[i];
    }
    printf("\n%d\n", sum);

    // int cntp = 0, cntf=0;
    // for(int i = stop+1;i<stop+CUCKOO_SIZE/20; ++i){
    //     uint64_t ret;
    //     if(cuckoo.insert(kv[i], &ret) == false){
    //         cntf++;
    //         if(ret > stop)
    //             cntp++;
    //         if(i%100000 == 0)
    //             printf("~~ %llu\n",ret);
    //     }
    // }
    // printf("cntf: %d, cntp: %d\n", cntf, cntp);
    // cuckoo.print_loadfactor();

    // printf("debug:%d\n",cuckoo.debug);
    delete_kv();
    return 0;
}