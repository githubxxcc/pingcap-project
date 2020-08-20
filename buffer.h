//
// Created by Chen Xu on 8/19/20.
//
#include <vector>
#include <unordered_map>
#include <atomic>
#include <list>
#include <string>

#ifndef PINGCAP_INTERVIEW_BUFFER_H
#define PINGCAP_INTERVIEW_BUFFER_H

using page_id_t = uint16_t;
using offset_t = size_t;
const size_t PAGE_SIZE = 4 * 1024; // 4Kb
const size_t MAX_WORD_SIZE_PAGE = 32; // 32b
const page_id_t INVALID_PAGE_ID = 0;

//class Disk {
//public:
//    Disk(const std::string &disk_file) {
//
//    }
//
//private:
//    std::fstream io_;
//    std::string file_name_;
//};

/**
 * Block that represents a page to store data
 */

const size_t PAGE_HEADER_SIZE = sizeof(page_id_t) + sizeof(size_t) *3;


class Page {
public:
    Page() {
        memset(data_, 0, PAGE_SIZE - PAGE_HEADER_SIZE);
    }

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

    bool HasLine(const std::string &word, int *word_id_ptr, size_t * offset_ptr = nullptr) {

        auto size_offset = GetWordSizeOffset();
        for(size_t i = 0; i < num_word_ ; i++) {
            auto offset = size_offset[i];
            auto word_size = GetWordSizeAtOffset(i, size_offset);
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
        if((word_size + sizeof(int) + size_occupied_) > PAGE_SIZE) {
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

            size_occupied_ += sizeof(int) + word.size();
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

    bool AddPage(page_id_t page_id) {
        return false;
    }

    page_id_t EvictPage() {
        return 1;
    }

private:
    std::unordered_map<page_id_t, std::list<page_id_t>::iterator> lru_map_;
    std::list<page_id_t> lru_list_;
};

class BufferManager {
public:
    BufferManager()
            :replacer_(){
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
        return data_.at(page_id);
    }

    bool FlushPage(page_id_t page_id) {
        return true;
    }

    void NewPage(page_id_t *page_id_ptr) {
        Page * new_page = new Page();
        *page_id_ptr = next_page_id_;
        data_.insert({next_page_id_, new_page});
        new_page->InitializePage(next_page_id_);
        next_page_id_++;

        // TODO(real allocation)
    }


private:

    // In-memmory version
    std::unordered_map<page_id_t, Page*> data_;

    std::unordered_map<page_id_t , offset_t> page_map;
    Page *pages_;
    Replacer replacer_;
    page_id_t  next_page_id_ = 1;
};

#endif //PINGCAP_INTERVIEW_BUFFER_H
