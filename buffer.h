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
class Page {
public:
    std::pair<std::string, int> GetMinUnique() {
        std::string min_unique;
        int min_unique_id = INT_MAX;

        for(auto &p: words_) {
            if(p.second< min_unique_id) {
                min_unique = p.first;
                min_unique_id = p.second;
            }
        }

        return {min_unique, min_unique_id};
    }

    bool HasLine(const std::string &word, int *word_id_ptr) {
        if(words_.count(word)) {
            // Already have
            if(word_id_ptr != nullptr){
                *word_id_ptr = abs(words_.at(word));
            }
            return true;
        }

        return false;
    }

    bool HasSpaceFor(const std::string &word) {
        if(word.size() + size_occupied_ > MAX_WORD_SIZE_PAGE) {
            return false;
        }

        return true;
    }

    void AddWord(const std::string &word, int word_id) {
        if(HasLine(word, nullptr)) {
            words_.at(word) = 0-abs(word_id);
        } else {
            words_[word] = word_id;
        }
    }

private:
    std::unordered_map<std::string, int> words_;
    size_t size_occupied_ = 0;
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
        next_page_id_++;

        // TODO(real allocation)
    }


private:

    // In-memmory version
    std::unordered_map<page_id_t, Page*> data_;

    std::unordered_map<page_id_t , offset_t> page_map;
    Page *pages_;
    Replacer replacer_;
    page_id_t  next_page_id_ = 0;
};

#endif //PINGCAP_INTERVIEW_BUFFER_H
