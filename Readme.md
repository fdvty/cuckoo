# Linear Probing Cuckoo Hashing (LPCH) 华为哈希表

---

© PKU 1806

## 算法简介

### 算法定位

LPCH是一个用来管理大量Key-Value Pairs的哈希表，它的装载率可以达到90%以上。并且在装载率90%以上时可以保证很高的更新速度和查询速度。

### Cuckoo Hashing

传统的Cuckoo Hashing由两个表T<sub>1</sub>、T<sub>2</sub>和两个哈希函数h<sub>1</sub>、h<sub>2</sub>构成。一个元素e = <k, v>在哈希表中被存放在表T<sub>1</sub>的h<sub>1</sub>(k)处或表T<sub>2</sub>的h<sub>2</sub>(k)处。

当要插入e<sub>0</sub> = <k<sub>0</sub>, v<sub>0</sub>>时，计算h<sub>1</sub>(k<sub>0</sub>)，然后把e<sub>0</sub>存到T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处。如果T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处原来没有元素，则插入成功。否则，设那里原来的元素是e<sub>1</sub> = <k<sub>1</sub>, v<sub>1</sub>>。那么接下来要把e<sub>1</sub> = <k<sub>1</sub>, v<sub>1</sub>>插入到表T<sub>2</sub>的h<sub>2</sub>(k<sub>1</sub>)处。同样地，如果那里原来没有元素，则插入成功。否则把原来的元素再踢到T<sub>1</sub>，如此反复······

上面的kick操作可能陷入死循环，所以cuckoo hashing为插入操作设置了一个kick的上限。插入一个元素时如果kick的次数超过了这个上限则插入失败。

当要查询k<sub>0</sub>时，计算h<sub>1</sub>(k<sub>0</sub>)和h<sub>2</sub>(k<sub>0</sub>)，然后在T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处和T<sub>2</sub>的h<sub>2</sub>(k<sub>0</sub>)处查找k<sub>0</sub>。如果找到了就返回对应的kv pair，如果没找到则查询失败。

### Linear Probing Cuckoo Hashing

传统Cuckoo Hashing的装载率只有50%-60%，而且当插入失败时会有元素丢失。我们的算法针对Cuckoo Hashing的上述缺陷做出了改进。

（1）在插入元素e<sub>0</sub> = <k<sub>0</sub>, v<sub>0</sub>>时，线性探查表T<sub>1</sub>的从h<sub>1</sub>(k<sub>0</sub>)开始的m<sub>1</sub>个位置，如果这些位置都满了，再把T<sub>1</sub>的h<sub>1</sub>(k<sub>0</sub>)处的元素踢到T<sub>2</sub>，为e<sub>0</sub>腾地方。同样地，在向表T<sub>2</sub>中插入元素时也要线性探查m<sub>2</sub>个位置。自然地，我们对查询操作也做出相应的修改。

这个改进会降低插入和查询操作的速度，但是可以显著提升装载率。改进后的装载率能达到90%以上。

（2）在插入过程中，假设已经达到了kick的上限，设e = <k, v>是最后要被踢到另一个表的元素。传统的Cuckoo Hashing直接丢弃这个元素。我们这里为表T<sub>2</sub>的每个表项增加一个指针域，指向一个链表。在插入失败时，我们把e = <k, v>挂在表T<sub>2</sub>的h<sub>2</sub>(k)位置的链表上。

这个改进会让T<sub>2</sub>产生额外的空间开销，也会降低一点查询速度，但是可以保证插入操作不会丢弃元素。在我们的设计中，T<sub>1</sub>、T<sub>2</sub>的size大概为3:1。


## 实验结果

#### 实验1 

##### 实验平台

处理器 2.5GHz Intel Core i7 (4核)

内存 16 GB 1600 MHz DDR3

L2缓存 256 KB， L3缓存 6 MB

操作系统 macOS Mojave 10.14.6

##### 数据集及哈希表配置

随机生成10,000,000个key-value pair，key的长度为20字节，value的长度为10字节。

哈希函数采用BOB Hash（http://burtleburtle.net/bob/hash/evahash.html）

LPCH的总大小也为10,000,000，T<sub>1</sub>和T<sub>2</sub>的size的比为3:1。m<sub>1</sub> = 9，m<sub>2</sub> = 3。kick的上限为30次。插入失败（挂链表）200次时停止插入，计算装载率。

##### 实验结果

下面是上述配置下运行10次的实验结果，插入速度和查询速度用MIPS（每秒百万次操作）度量。

load factor: 0.914991, insert MIPS: 1.912497, query MIPS: 5.778562

load factor: 0.915424, insert MIPS: 2.046010, query MIPS: 5.676418

load factor: 0.916445, insert MIPS: 2.050040, query MIPS: 5.785639

load factor: 0.915884, insert MIPS: 2.083705, query MIPS: 5.780324

load factor: 0.916482, insert MIPS: 1.959996, query MIPS: 5.604952

load factor: 0.915468, insert MIPS: 1.943140, query MIPS: 5.339390

load factor: 0.914829, insert MIPS: 1.927954, query MIPS: 5.711250

load factor: 0.916446, insert MIPS: 1.988155, query MIPS: 5.688728

load factor: 0.915661, insert MIPS: 1.966910, query MIPS: 5.653916

load factor: 0.915409, insert MIPS: 1.897667, query MIPS: 5.579936

## 运行说明

1. git clone https://github.com/fdvty/cuckoo.git

2. cd cuckoo

3. make clean; make

4. ./cuckoo


## 目录结构
```
├── Readme.md                   // help
├── cuckoo                      // 应用
├── cuckoo.h                    // LPCH 算法
├── BOBHash32.h                 // 哈希函数
├── utils.h                     // 用于创建Key-Value Pair的函数
├── main.cpp
└── Makefile
```





