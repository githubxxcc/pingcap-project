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

    ASSERT_NE(buffer.FetchPage(page_id), nullptr);
}

TEST_F(BufferTest, test_add)
{
    BufferManager buffer = BufferManager();

    page_id_t  page_id;
    buffer.NewPage(&page_id);

    auto *page = buffer.FetchPage(page_id);

    page->AddWord("ping-cap", 1);
    std::string s = "ping-cap";
    ASSERT_EQ(page->GetNumWord(), 1);
    ASSERT_EQ(page->GetNextWordEnd(), PAGE_SIZE - PAGE_HEADER_SIZE - s.size() - sizeof(int));
    int id = -1;

    // Should exist with word_id = 1
    ASSERT_TRUE(page->HasLine("ping-cap", &id));
    ASSERT_EQ(id, 1);
}

class FinderTest : public ::testing::Test
{
};

TEST_F(FinderTest, test_add) {
    auto buffer = std::make_unique<BufferManager>();
    Finder finder = Finder(std::move(buffer), "test.txt", 8 * 1024);
    EXPECT_NO_THROW(finder.AddString("ping-cap"));

    EXPECT_NO_THROW(finder.AddString("ping-cap1"));
    EXPECT_NO_THROW(finder.AddString("ping-cap"));
    EXPECT_NO_THROW(finder.AddString("ping-cap2"));

    EXPECT_EQ(finder.GetFirstUnique(), "ping-cap1");
}
