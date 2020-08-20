#include <iostream>

#include "buffer.h"

int main(int argc, char *argv[])
{

    // Parse command line args
    if(argc != 2) {
        std::cerr << "USAGE: " << argv[0] << " INPUT_FILE " << std::endl;
        return 1;
    }

    std::string input_file(argv[1]);

    // TODO(CHECK FILE)
    auto page_buffers = std::unique_ptr<BufferManager>();

    return 0;
}