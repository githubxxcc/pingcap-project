//
// Created by Chen Xu on 8/19/20.
//

#include <gtest/gtest.h>
#include "buffer.h"
#include "finder.h"

class BufferTest : public ::testing::Test
{
};

TEST_F(BufferTest, test_new)
{
    BufferManager buffer = BufferManager();

    page_id_t  page_id;
    buffer.NewPage(&page_id);
    EXPECT_EQ(buffer.FetchPage(page_id)->GetPageId(), page_id);
    EXPECT_NE(buffer.FetchPage(page_id), nullptr);
}

TEST_F(BufferTest, test_add)
{
    BufferManager buffer = BufferManager();

    page_id_t  page_id;
    buffer.NewPage(&page_id);

    auto *page = buffer.FetchPage(page_id);

    page->AddWord("ping-cap", 1);
    std::string s = "ping-cap";
    EXPECT_EQ(page->GetNumWord(), 1);
    EXPECT_EQ(page->GetNextWordEnd(), PAGE_SIZE - PAGE_HEADER_SIZE - s.size() - sizeof(int));
    int id = -1;

    // Should exist with word_id = 1
    EXPECT_TRUE(page->HasLine("ping-cap", &id));
    EXPECT_EQ(id, 1);
}

TEST_F(BufferTest, test_replace) {

    BufferManager buffer = BufferManager(2);
    page_id_t page_id_1;
    buffer.NewPage(&page_id_1);

    page_id_t page_id_2;
    buffer.NewPage(&page_id_2);

    page_id_t page_id_3;
    buffer.NewPage(&page_id_3);

    auto page_in_memory = buffer.GetInMemoryPages();
    EXPECT_EQ(page_in_memory.size(), 2);
    EXPECT_EQ(page_in_memory.count(page_id_2), 1);
    EXPECT_EQ(page_in_memory.count(page_id_3), 1);
}

class FinderTest : public ::testing::Test
{
};

TEST_F(FinderTest, test_add) {
    auto buffer = std::make_unique<BufferManager>();
    Finder finder = Finder(std::move(buffer), "test.txt", 8 * 1024);
    EXPECT_NO_THROW(finder.AddWord("ping-cap"));

    EXPECT_NO_THROW(finder.AddWord("ping-cap1"));
    EXPECT_NO_THROW(finder.AddWord("ping-cap"));
    EXPECT_NO_THROW(finder.AddWord("ping-cap2"));

    EXPECT_EQ(finder.GetFirstUnique(), "ping-cap1");
}

class ReplacerTest: public ::testing::Test {};

TEST_F(ReplacerTest, test_lru) {
    Replacer replacer;
    replacer.AddPage(1);
    replacer.AddPage(2);
    EXPECT_EQ(replacer.EvictPage(), 1);

    replacer.AddPage(1);
    EXPECT_EQ(replacer.EvictPage(), 2);

    replacer.AddPage(2); // 2 -> 1
    replacer.AddPage(3); // 3 -> 2 -> 1
    replacer.AddPage(4); // 4 -> 3 -> 2 -> 1

    replacer.TouchPage(1); // 1 -> 4 -> 3 -> 2
    replacer.TouchPage(2); // 2 -> 1 -> 4 -> 3

    EXPECT_EQ(replacer.EvictPage(), 3);
    EXPECT_EQ(replacer.EvictPage(), 4);
    EXPECT_EQ(replacer.EvictPage(), 1);
    EXPECT_EQ(replacer.EvictPage(), 2);
}

class DiskTest: public ::testing::Test {};

TEST_F(DiskTest, test_disk)
{
    Disk disk("/tmp/test.db");
    EXPECT_EQ(disk.AllocatePage(), 0);
    EXPECT_EQ(disk.AllocatePage(), PAGE_SIZE);
    EXPECT_EQ(disk.AllocatePage(), 2*PAGE_SIZE);

    Page *page1 = new Page();
    page1->InitializePage(1);
    Page *page2 = new Page();
    page2->InitializePage(2);
    Page *page3 = new Page();
    page3->InitializePage(3);

    disk.WritePage(page1, 0);
    disk.WritePage(page2, PAGE_SIZE);
    disk.WritePage(page3, 2*PAGE_SIZE);

    Page *dummy = new Page();
    disk.ReadPage(dummy, 0);
    EXPECT_EQ(dummy->GetPageId(), 1);

    disk.ReadPage(dummy, PAGE_SIZE);
    EXPECT_EQ(dummy->GetPageId(), 2);

    disk.ReadPage(dummy, 2*PAGE_SIZE);
    EXPECT_EQ(dummy->GetPageId(), 3);

    delete page1;
    delete page2;
    delete page3;
    delete dummy;
}


