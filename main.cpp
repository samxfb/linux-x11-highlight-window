#include "highlight_window.h"
#include <iostream>

int main(int argc, char *argv[]) {
    int border_width_ = 6;
    int border_length_ = 120;
    int64_t border_color_ = 0xFF29CCA3; // argb
    if (argc == 2) {
        // nothing to do
    } else if (argc == 3) {
        border_width_ = atoi(argv[2]);
    } else if (argc == 4) {
        border_width_ = atoi(argv[2]);
        border_length_ = atoi(argv[3]);
    } else if (argc == 5) {
        border_width_ = atoi(argv[2]);
        border_length_ = atoi(argv[3]);
        border_color_ = std::stoll(argv[4], nullptr, 16);
    } else {
        std::cout << "Usage: " << argv[0] << " <window_id>[required] <border_width>[optional] <border_length>[optional] <border_color:0xAARRGGBB>[optional]" << std::endl;
        return -1;
    }

    Window window_ = atoi(argv[1]);

    HighLightWindow highlight_window(window_, border_width_, border_length_, border_color_);
    highlight_window.Start();

    std::cout << "Press any key to exit..." << std::endl;
    getchar();

    highlight_window.Stop();
    
    return 0;
}