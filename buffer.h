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
    std::pair<std::string, int> GetMinUnique() {
        std::string min_unique;
        int min_unique_id = INT_MAX;

        auto size_offset = GetWordSizeOffset();
        for(size_t i = 0; i < num_word_ ; i++) {
            auto offset = size_offset[i];
            auto word_size = GetWordSizeAtOffset(i, size_offset);
            std::pair<std::string, int> str_id = GetWordWithId(offset, word_size);
            if(str_id.second >= 0 && str_id.second < min_unique_id) {
                min_unique_id = str_id.second;
                min_unique = str_id.first;
            }
        }

        return {min_unique, min_unique_id};
    }

    void InitializePage(page_id_t page_id) {
        page_id_ = page_id;
    }

    page_id_t GetPageId() const { return page_id_;}

    bool HasLine(const std::string &word, int *word_id_ptr, size_t * offset_ptr = nullptr) {

        auto size_offset = GetWordSizeOffset();
        for(size_t i = 0; i < num_word_ ; i++) {
            auto offset = size_offset[i];
            auto word_size = GetWordSizeAtOffset(i, size_offset);
            if(word_size > 10000) {

            }
            std::pair<std::string, int> str_id = GetWordWithId(offset, word_size);
            if(str_id.first == word) {
                if(word_id_ptr != nullptr)
                    *word_id_ptr = abs(str_id.second);

                if(offset_ptr != nullptr)
                    *offset_ptr = offset;

                return true;
            }
        }

        return false;
    }

    bool HasSpaceFor(const std::string &word) {
        size_t word_size = word.size();
        if((word_size + sizeof(int) + sizeof(size_t) + size_occupied_) > PAGE_SIZE) {
            return false;
        }
        return true;
    }

    void AddWord(const std::string &word, int word_id) {
        size_t offset;
        if(HasLine(word, nullptr, &offset)) {
            // Has the word, just change the word_id to negative
            SetWordId(offset, word.size(), 0-abs(word_id));
        } else {
            // No word present, add the new word
            assert(next_word_end_ >= word.size() + sizeof(int));
            auto offset = next_word_end_ - word.size() - sizeof(int);
            memcpy(data_+offset,word.c_str(), word.size());
            memcpy(data_+offset+word.size(), &word_id, sizeof(int));

            size_occupied_ += sizeof(int) + word.size() + sizeof(size_t);
            auto* size_offset = GetWordSizeOffset();
            size_offset[num_word_] = offset;
            num_word_++;
            next_word_end_ = offset;
        }
    }

    inline size_t GetNumWord() const { return num_word_;}

    inline size_t GetNextWordEnd() const {return next_word_end_;}

private:
    inline size_t * GetWordSizeOffset() {
        return reinterpret_cast<size_t*>(data_);
    }

    inline std::pair<std::string, int> GetWordWithId(size_t offset, size_t word_size) {
        std::string word(data_ + offset, word_size);
        int word_id = *(reinterpret_cast<int*>(data_ + offset + word_size));

        return {word, word_id};
    }

    inline void SetWordId(size_t offset, size_t word_size, int word_id) {
        int* word_id_ptr = reinterpret_cast<int*>(data_ + offset + word_size);
        *word_id_ptr = word_id;
    }


    inline size_t GetWordSizeAtOffset(size_t i, size_t *size_offset) {
        if(i == 0) { // First word
            return PAGE_SIZE - PAGE_HEADER_SIZE - size_offset[i] - sizeof(int);
        } else {
            return size_offset[i-1] - size_offset[i] - sizeof(int);
        }
    }

    size_t size_occupied_ = PAGE_HEADER_SIZE;
    size_t num_word_ = 0;
    size_t next_word_end_ = PAGE_SIZE - PAGE_HEADER_SIZE;
    page_id_t  page_id_ = INVALID_PAGE_ID;

    char data_[PAGE_SIZE - PAGE_HEADER_SIZE]{};
};

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
     * Fetch a page from the disk
     * @param pageid
     * @return
     */
    Page* FetchPage(page_id_t page_id) {
        if(data_.count(page_id)) {
           // In memory
           replacer_.TouchPage(page_id);
           return data_.at(page_id);
        }

        // On disk
        assert(page_map_.count(page_id));
        auto offset = page_map_.at(page_id);

        // Evict a page if memory full
        if(data_.size() == max_page_num_) {
            EvictPage();
        }
        assert(data_.size() < max_page_num_);

        // Read the page
        auto *new_page = new Page();
        disk_.ReadPage(new_page, offset);
        data_.insert({new_page->GetPageId(), new_page});
        replacer_.AddPage(new_page->GetPageId());

        return new_page;
    }

    void NewPage(page_id_t *page_id_ptr) {
        if(data_.size() == max_page_num_) {
            EvictPage();
        }

        Page * new_page = new Page();
        *page_id_ptr = next_page_id_;
        data_.insert({next_page_id_, new_page});
        new_page->InitializePage(next_page_id_);

        replacer_.AddPage(new_page->GetPageId());

        // Reserve place on the disk
        auto offset = disk_.AllocatePage();
        page_map_.insert({next_page_id_, offset});

        next_page_id_++;
    }

    std::unordered_map<page_id_t, Page*> GetInMemoryPages() const {return data_;}

    std::unordered_map<page_id_t , size_t> GetOffsetMap() const {return page_map_;}

private:
    void EvictPage() {
        auto evict_page_id = replacer_.EvictPage();
        assert(data_.count(evict_page_id));

        auto* page = data_.at(evict_page_id);
        disk_.WritePage(page, page_map_.at(evict_page_id));

        // Clear the in-memory
        delete page;
        data_.erase(evict_page_id);
    }

    // In-memory pages
    std::unordered_map<page_id_t, Page*> data_;
    std::unordered_map<page_id_t , size_t> page_map_;
    Replacer replacer_;
    Disk disk_;
    page_id_t  next_page_id_ = 1;
    const size_t max_page_num_;
};

#endif //PINGCAP_INTERVIEW_BUFFER_H
