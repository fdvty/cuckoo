# Linear Probing Cuckoo Hashing (LP-Cuckoo)



## 用法

把```cuckoo.h```和```murmur3.h```复制到工作目录下，```#include"cuckoo.h"```，创建一个```Cuckoo```类，然后调用它的public接口函数```insert```，```search```，```remove```即可。

推荐用g++，-O3编译。







# About this repo


## 算法

### 定位

LP-Cuckoo是一个用来管理大量KV Pairs的哈希表，它的装载率可以达到90%以上。并且在装载率90%以上时可以保证很高的更新速度和查询速度。

### Cuckoo Hashing

Cuckoo Hashing由两个表T<sub>1</sub>、T<sub>2</sub>和两个哈希函数h<sub>1</sub>、h<sub>2</sub>构成。规定元素e = <k, v>在哈希表中被存放在表T<sub>1</sub>的h<sub>1</sub>(k)处或表T<sub>2</sub>的h<sub>2</sub>(k)处。

当要插入e<sub>0</sub> = <k<sub>0</sub>, v<sub>0</sub>>时，计算h<sub>1</sub>(k<sub>0</sub>)，然后把e<sub>0</sub>存到T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处。如果T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处原来没有元素，则插入成功。否则，设那里原来的元素是e<sub>1</sub> = <k<sub>1</sub>, v<sub>1</sub>>。那么接下来要把e<sub>1</sub> = <k<sub>1</sub>, v<sub>1</sub>>插入到表T<sub>2</sub>的h<sub>2</sub>(k<sub>1</sub>)处。同样地，如果那里原来没有元素，则插入成功。否则把原来的元素再踢到T<sub>1</sub>，如此反复······

上面的kick操作可能陷入死循环，所以cuckoo hashing为插入操作设置了一个kick的上限。插入一个元素时如果kick的次数超过了这个上限则插入失败。

当要查询k<sub>0</sub>时，计算h<sub>1</sub>(k<sub>0</sub>)和h<sub>2</sub>(k<sub>0</sub>)，然后在T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处和T<sub>2</sub>的h<sub>2</sub>(k<sub>0</sub>)处查找k<sub>0</sub>。如果找到了就返回对应的kv pair，如果没找到则查询失败。

### Linear Probing Cuckoo Hashing

Cuckoo Hashing要做到高装载率需要把kick的次数设得很高，这会让更新速度变得很慢。而且Cuckoo Hashing当插入失败时会有元素丢失。我们的算法针对Cuckoo Hashing的上述缺陷做出了改进。

在插入元素e<sub>0</sub> = <k<sub>0</sub>, v<sub>0</sub>>时，线性探查表T<sub>1</sub>的从h<sub>1</sub>(k<sub>0</sub>)开始的m<sub>1</sub>个位置，和T<sub>2</sub>的从h<sub>2</sub>(k<sub>0</sub>)开始的m<sub>2</sub>个位置，如果这些位置都满了，再把T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处的元素踢到T<sub>2</sub>，为e<sub>0</sub>腾地方。同样地，在向表T<sub>2</sub>中插入元素时也要线性探查m<sub>2</sub>个位置。自然地，对查询操作也做出相应的修改。

这个改进可以显著提升装载率。改进后的装载率能达到90%以上。





## Evaluations

### Experimental setup

#### Platform

Intel(R) Core(TM) i9-10980XE CPU @ 3.00GHz

We implement all codes with c++ and build them with g++ 7.5.0 and -O3 option. 


#### Workload
We randomly generate 10,000,000 key-value pairs, where the size of key/value in each pair is 8Bytes.  


#### Hash functions and parameters
We use MurmurHash for hash calculations. 
We fix the size of LP-Cuckoo to 10,000,000 slots, where T<sub>1</sub> has 7,500,000 slots and T<sub>2</sub> has 2,500,000 slots. 
We set m<sub>1</sub> = 8, m<sub>2</sub> = 8, and set the kick threshold to 35. 
We sequentially insert the 10,000,000 key-value pairs until the 500<sub>th</sub> insertion failure. 
Then we calculate the load factor. 


### Results


load factor: 95.61% ;

insert speed: 10.19M/s (average), 4.29M/s (load factor >90%) ; 

query speed: 18.84M/s (positive), 13.60M/s (negative) ;





## How to run

1. git clone https://github.com/fdvty/cuckoo.git

2. cd cuckoo

3. make clean; make

4. ./cuckoo



## Directory
```
├── Readme.md                   
├── cuckoo                      
├── cuckoo.h                    // *LP-Cuckoo 算法
├── murmur3.h                   // *哈希函数
├── utils.h                     // 用于随机生成Key-value pair的函数
├── main.cpp
└── Makefile
```





