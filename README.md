# pingcap-inter-project

### Build 
```
mkdir build
cd build && cmake ..
make 
```

### Test 
```
cd build && ./unit_tests
```

### Run 
```
# 生成测试数据 
python generate_data.py

# 找出测试数据中第一个不重复的词
cd build && ./pingcap_interview ../test.txt 
```


### Design
由于内存限制（16G）比文本（100G）要小，所以一部分的数据必须得储存到disk上。
数据将以一个4KB的Page为单位被储存在内存或者硬盘上。

#### Page 结构
一个Page将由以下的结构：
```
|  num_word , next_size_offset, next_word_offset, page_id | <---- Page Header
|  word1_offset, word2_offset , ....                      | <---- Word Offsets
|  .....                                                  | 
|  .....                word2, word2_id, word1, word1_id  | <---- Words

```
 - num_word: Page中存了的word数量
 - next_size_offset: 关于每个Word大小（offset）的信息的最后一个的位置, e.g.: wordk_offset 的位置
 - next_word_offset: 下一个word该被存的offset，e.g. wordk 的位置
 - page_id: Page ID
 - word1_offset: word1 + word1_id 开始的offset
 - word1: Word1的文本
 - word1_id： Word1的word id
 
 比如一个包含了2个单词的（ping，cap）的Page将会是以下的结构：
 ```
 | 2, 24, 4081, 1   | 
 | 4088, 4081, ..   |
 |..., cap,2,ping,1 |
 
 ```
 - 2: 2个单词
 - 24: 下一个offset开始的地方（header为16byte， 4088占用了4个byte， 4081占用了4个byte， 所以24 = 16+4+4）
 - 4081: 下一个单词结束的offset (4081 = 4096 - 3 - 4 - 4 -4):
   - cap: 3byte
   - 2: 4byte
   - ping: 4byte
   - 1: 4byte
   
#### 去重
当有发现重复的单词时候，将其对应的word_id变为负数。 所以最后那个最小的正的wordid的单词，就是第一个不重复的单词。
 

#### 内存中的信息
 - 在内存里的Page们的ID
 - Page在Disk上Offset的信息
 - 能放在内存中的Page
 
#### Spill-over硬盘
当内存中的Page的数量不够用了的时候，Page会被存到Disk上。

利用一个LRU的缓存替换算法，将最早使用过的Page给存到Disk上。


### 优化
1. 利用Trie结构来压缩文本的存储（IO优化）
2. 当重复多次的词出现时候，不需要对已经变为负数的wordid进行更新。对于没有被更新的Page，不需要再写回到Disk（IO优化）

 
 

