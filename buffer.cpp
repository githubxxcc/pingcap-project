//
// Created by Chen Xu on 8/19/20.
//

#include "buffer.h"
std::pair<std::string, int> Page::GetMinUnique() {
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

bool Page::HasLine(const std::string &word, int *word_id_ptr, size_t * offset_ptr) {
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
void Page::AddWord(const std::string &word, int word_id) {
    size_t offset = next_word_end_;
    if(HasLine(word, nullptr, &offset)) {
        // Has the word, just change the word_id to negative
        SetWordId(offset, word.size(), 0-abs(word_id));
    } else {
        // No word present, add the new word
        if(next_word_end_ < word.size() + sizeof(int)) {
            std::cout<<next_word_end_ << std::endl;
            std::cout<<word.size() << std::endl;
        }
        assert(next_word_end_ >= word.size() + sizeof(int));
        offset = next_word_end_ - word.size() - sizeof(int);
        memcpy(data_+offset,word.c_str(), word.size());
        memcpy(data_+offset+word.size(), &word_id, sizeof(int));

        next_size_offset_ += sizeof(size_t);
        auto* size_offset = GetWordSizeOffset();
        size_offset[num_word_] = offset;
        num_word_++;
        next_word_end_ = offset;
        assert(next_word_end_ > next_size_offset_);
    }
}

// BufferManager
Page* BufferManager::FetchPage(page_id_t page_id) {
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


void BufferManager::NewPage(page_id_t *page_id_ptr) {
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


void BufferManager::EvictPage() {
    auto evict_page_id = replacer_.EvictPage();
    assert(data_.count(evict_page_id));

    auto* page = data_.at(evict_page_id);
    disk_.WritePage(page, page_map_.at(evict_page_id));

    // Clear the in-memory
    delete page;
    data_.erase(evict_page_id);
}
