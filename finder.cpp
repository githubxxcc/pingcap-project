//
// Created by Chen Xu on 8/19/20.
//

#include "finder.h"
#include <functional>

void Finder::AddWord(const std::string &word) {
    // Find the bucket
    auto hash = std::hash<std::string>{}(word);
    auto bucket_id = hash % num_buckets_;

    auto &pages = buckets_[bucket_id];
    int word_id = next_word_id_;
    auto *page = GetTargetPage(pages, word, &word_id);
    if(word_id == next_word_id_) next_word_id_++;

    page->AddWord(word, word_id);

    return;
}

std::string Finder::GetFirstUnique() {
    int min_id = INT_MAX;
    std::string ans;
    for(const auto &bucket: buckets_) {
        for(auto page_id: bucket) {
            Page* page = buffer_->FetchPage(page_id);
            auto min_unique_pair = page->GetMinUnique();

            if(min_id > min_unique_pair.second) {
                min_id = min_unique_pair.second;
                ans = min_unique_pair.first;
            }
        }
    }

    return ans;
}

Page* Finder::GetTargetPage(std::vector<page_id_t> &page_ids, const std::string &word, int *word_id_ptr) {
    if(page_ids.empty()) {
        page_id_t page_id = InitializePage();
        page_ids.push_back(page_id);
    }

    auto free_page_id = INVALID_PAGE_ID;
    for(auto page_id : page_ids) {
        // Look for duplicates
        auto * page = buffer_->FetchPage(page_id);
        if(page->HasLine(word, word_id_ptr)) {
            return page;
        }

        // Look for page that can contain the word
        if(free_page_id == INVALID_PAGE_ID && page->HasSpaceFor(word)) {
            free_page_id = page->GetPageId();
        }
    }

    // No free page
    if(free_page_id == INVALID_PAGE_ID) {
        free_page_id = InitializePage();
        page_ids.push_back(free_page_id);
    }

    return buffer_->FetchPage(free_page_id);
}

page_id_t Finder::InitializePage() {
    // Allocate on the buffer
    page_id_t page_id;
    buffer_->NewPage(&page_id);

    return page_id;
}


std::string Finder::Process() {
    if(!input_.is_open()) return "";

    std::string line;
    // Add all the words
    while(std::getline(input_, line)) {
        AddWord(line);
    }

    // Return the first unique string
    return GetFirstUnique();

}
