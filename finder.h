//
// Created by Chen Xu on 8/19/20.
//

#ifndef PINGCAP_INTERVIEW_FINDER_H
#define PINGCAP_INTERVIEW_FINDER_H

#include "buffer.h"
#include <string>
#include <fstream>

class PageInfo {

};

class Finder {
public:
    Finder(std::unique_ptr<BufferManager> buffer_manager, const std::string &input_file, size_t mem_limit)
            :
            buffer_(std::move(buffer_manager)),
            input_file_(input_file),
            mem_limit_(mem_limit),
            num_buckets_(mem_limit / PAGE_SIZE),
            buckets_(mem_limit/PAGE_SIZE, std::vector<page_id_t>())
    {
        input_.open(input_file_);
    }

    std::string FinderUnique() {
        if(!input_.is_open()) return "";

        std::string line;
        // Add all the words
        while(std::getline(input_, line)) {
            AddString(line);
        }

        // Return the first unique string
        return GetFirstUnique();
    }

    void AddString(const std::string &word);

    std::string GetFirstUnique();

private:

    Page* GetTargetPage(std::vector<page_id_t> &page_ids, const std::string &word, int *word_id);

    void AddToPage(const std::string &word, Page* page);


    page_id_t InitializePage();

    std::unique_ptr<BufferManager> buffer_;
    const std::string input_file_;
    std::ifstream input_;
    std::vector<std::vector<page_id_t>> buckets_;
    size_t mem_limit_;
    size_t num_buckets_;
    std::unordered_map<page_id_t, PageInfo> bucket_info_;
    int next_word_id_ = 0;
};

#endif //PINGCAP_INTERVIEW_FINDER_H
