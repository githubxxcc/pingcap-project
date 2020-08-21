#include <iostream>

#include "buffer.h"
#include "finder.h"

int main(int argc, char *argv[])
{

    // Parse command line args
    if(argc != 2) {
        std::cerr << "USAGE: " << argv[0] << " INPUT_FILE " << std::endl;
        return 1;
    }

    std::string input_file(argv[1]);

    // TODO(CHECK FILE)
    size_t mem_limit = 80*1024; // 80KB
    auto page_buffers = std::make_unique<BufferManager>(mem_limit/PAGE_SIZE);
    Finder finder(std::move(page_buffers), input_file, mem_limit); // 16Kb
    std::cout << "First Unique String: " << finder.FinderUnique() << std::endl;

    return 0;
}