//
// Created by Chen Xu on 8/19/20.
//

#ifndef PINGCAP_INTERVIEW_FINDER_H
#define PINGCAP_INTERVIEW_FINDER_H

#include "buffer.h"
#include <string>
#include <fstream>

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

    /**
     * Find the first unique string
     * @return
     */
    std::string Process();

    /**
     * Add a word to the finder
     * @param word
     */
    void AddWord(const std::string &word);

    /**
     * Get the first unique string from all the words
     * @return
     */
    std::string GetFirstUnique();

private:

    /**
     * Get the page where the word should be stored at
     * @param page_ids pages candidates in the bucket
     * @param word word to be added
     * @param word_id word_id to be used
     * @return the page where the word should be added
     */
    Page* GetTargetPage(std::vector<page_id_t> &page_ids, const std::string &word, int *word_id);

    page_id_t InitializePage();

    std::unique_ptr<BufferManager> buffer_;
    const std::string input_file_;
    std::ifstream input_;
    std::vector<std::vector<page_id_t>> buckets_;
    size_t mem_limit_;
    size_t num_buckets_;
    int next_word_id_ = 1;
};

#endif //PINGCAP_INTERVIEW_FINDER_H
