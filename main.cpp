#include <iostream>

#include "buffer.h"
#include "finder.h"

int main(int argc, char *argv[])
{

    // Parse command line args
    if(argc < 2) {
        std::cerr << "USAGE: " << argv[0] << " INPUT_FILE [MEM_LIMIT_BYTES] " << std::endl;
        return 1;
    }

    std::string input_file(argv[1]);
    size_t mem_limit = 80*1024; // 80KB by default
    if(argc == 3) {
        std::string mem_limit_str(argv[2]);
        mem_limit = std::stoi(mem_limit_str);
    }
    auto page_buffers = std::make_unique<BufferManager>(mem_limit/PAGE_SIZE);
    Finder finder(std::move(page_buffers), input_file, mem_limit); // 16Kb
    std::cout << "First Unique String: " << finder.Process() << std::endl;

    return 0;
}