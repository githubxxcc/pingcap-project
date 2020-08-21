//
// This file contains the interface between memory and disk
// Created by Chen Xu on 8/19/20.
//

#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <list>
#include <string>

#ifndef PINGCAP_INTERVIEW_BUFFER_H
#define PINGCAP_INTERVIEW_BUFFER_H

class Page;
class Disk;
class Replacer;
class BufferManager;

using page_id_t = uint16_t;
using offset_t = size_t;

const size_t PAGE_SIZE = 4 * 1024; // 4Kb
const size_t PAGE_HEADER_SIZE = sizeof(page_id_t) + sizeof(size_t) *3;
const page_id_t INVALID_PAGE_ID = 0;

/**
 * Disk that stores pages continuously on disk
 */
class Disk {
public:
    Disk(const std::string &disk_file) {
        io_.open(disk_file, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
        assert(io_.is_open());
    }

    ~Disk() {
        io_.close();
    }

    /**
     * Write a page's data to disk file at the offset
     * @param page
     * @param offset
     */
    void WritePage(const Page* page, size_t offset) {
        io_.seekp(offset);
        io_.write(reinterpret_cast<const char*>(page), PAGE_SIZE);
        io_.flush();
    }

    /**
     * Read a page from disk to memory at the offset
     * @param page
     * @param offset
     */
    void ReadPage(Page* page, size_t offset) {
        io_.seekg(offset);
        io_.read(reinterpret_cast<char*>(page), PAGE_SIZE);
    }

    /**
     * Allocate a new page on disk
     * @return the offset where the page is stored on the disk-file
     */
    size_t AllocatePage() {
        auto offset = next_offset_;
        next_offset_ += PAGE_SIZE;
        return offset;
    }

private:
    // file stream for read and write
    std::fstream io_;
    size_t next_offset_ = 0;
};




/**
 * Block that represents a page to store data
 * Page has the structure:
 * Page: [ PAGE_HEADER | DATA ]
 * For the data portion:
 * [ word1_offset | word2_offset | ... word2 + word_id | word1 + wordid ]
 */
class Page {
public:
    Page() {
        memset(data_, 0, PAGE_SIZE - PAGE_HEADER_SIZE);
    }

    /**
     * Get the first unique word in the Page
     * @return
     */
    std::pair<std::string, int> GetMinUnique();

    /**
     * Initialize a page by setting the page id
     * @param page_id
     */
    void InitializePage(page_id_t page_id) {
        page_id_ = page_id;
    }

    page_id_t GetPageId() const { return page_id_;}

    /**
     * Check if a line(word) has existed in the page
     * @param word word to check
     * @param word_id_ptr the id of the word to be set if word present
     * @param offset_ptr  the offset of the word if word present
     * @return if the word exists
     */
    bool HasLine(const std::string &word, int *word_id_ptr, size_t * offset_ptr = nullptr);

    /**
     * If there is enough space to store the word and its relevant information in the page
     * @param word
     * @return
     */
    bool HasSpaceFor(const std::string &word) {
        assert(next_word_end_ >= sizeof(int) + word.size());
        auto next_word_offset = next_word_end_ - sizeof(int) - word.size();
        auto next_size_offset = next_size_offset_ + sizeof(size_t);
        if(next_word_offset <= next_size_offset) {
            return false;
        }
        return true;
    }

    /**
     * Add a word to the page with word_id. It makes the word_id negative to indicate the
     * word is no longer unique
     * @param word
     * @param word_id
     */
    void AddWord(const std::string &word, int word_id);

    inline size_t GetNumWord() const { return num_word_;}

    inline size_t GetNextWordEnd() const {return next_word_end_;}

private:
    /**
     * Get the posistion where word offsets are stored
     * @return
     */
    inline size_t * GetWordSizeOffset() {
        return reinterpret_cast<size_t*>(data_);
    }

    /**
     * Get the word and its word_id
     * @param offset
     * @param word_size
     * @return
     */
    inline std::pair<std::string, int> GetWordWithId(size_t offset, size_t word_size) {

        std::string word(data_ + offset, word_size);
        int word_id = *(reinterpret_cast<int*>(data_ + offset + word_size));

        return {word, word_id};
    }

    inline void SetWordId(size_t offset, size_t word_size, int word_id) {
        int* word_id_ptr = reinterpret_cast<int*>(data_ + offset + word_size);
        *word_id_ptr = word_id;
    }

    /**
     * Compute the word's size based on its offset and its next word's offset
     * @param i current word index
     * @param size_offset beginning of size offset array
     * @return size of the word at the index
     */
    inline size_t GetWordSizeAtOffset(size_t i, size_t *size_offset) {
        if(i == 0) { // First word
            return PAGE_SIZE - PAGE_HEADER_SIZE - size_offset[i] - sizeof(int);
        }
        return size_offset[i-1] - size_offset[i] - sizeof(int);
    }

    size_t num_word_ = 0;
    size_t next_size_offset_ = PAGE_HEADER_SIZE;
    size_t next_word_end_ = PAGE_SIZE - PAGE_HEADER_SIZE;
    page_id_t  page_id_ = INVALID_PAGE_ID;

    char data_[PAGE_SIZE - PAGE_HEADER_SIZE]{};
};

/**
 * A LRU page replacer
 */
class Replacer {
public:
    Replacer() {}

    void AddPage(page_id_t page_id) {
        assert(!lru_map_.count(page_id));
        lru_list_.push_front(page_id);
        lru_map_.insert({page_id, lru_list_.begin()});
    }

    page_id_t EvictPage() {
        assert(!lru_list_.empty());
        auto evict_page_id = lru_list_.back();
        lru_list_.pop_back();
        lru_map_.erase(evict_page_id);

        return evict_page_id;
    }

    void TouchPage(page_id_t page_id) {
        assert(lru_map_.count(page_id));
        auto itr = lru_map_.at(page_id);
        lru_list_.erase(itr);
        lru_map_.erase(page_id);

        AddPage(page_id);
    }

private:
    std::unordered_map<page_id_t, std::list<page_id_t>::iterator> lru_map_;
    std::list<page_id_t> lru_list_;
};

/**
 * Buffer Pages Manager
 */
class BufferManager {
public:
    BufferManager(size_t page_num = 2)
            :replacer_(), disk_("/tmp/data.db"), max_page_num_(page_num){
    }

    ~BufferManager(){
        for(auto page_pair: data_) {
            delete page_pair.second;
        }
    }

    /**
     * Fetch a page from the disk/memory
     * @param pageid
     * @return
     */
    Page* FetchPage(page_id_t page_id);

    /**
     * Create a new page on the disk
     * @param page_id_ptr
     */
    void NewPage(page_id_t *page_id_ptr);

    std::unordered_map<page_id_t, Page*> GetInMemoryPages() const {return data_;}

    std::unordered_map<page_id_t , size_t> GetOffsetMap() const {return page_map_;}

private:
    /**
     * Evict a page from the buffer
     */
    void EvictPage();

    // In-memory pages
    std::unordered_map<page_id_t, Page*> data_;
    std::unordered_map<page_id_t , size_t> page_map_;
    Replacer replacer_;
    Disk disk_;
    page_id_t  next_page_id_ = 1;
    const size_t max_page_num_;
};

#endif //PINGCAP_INTERVIEW_BUFFER_H
