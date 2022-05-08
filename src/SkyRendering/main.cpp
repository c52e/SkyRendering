#include "AppWindow.h"

#include <iostream>
#include <stdexcept>

// Run with Nvidia GPU on laptop
extern "C" {
    _declspec(dllexport) unsigned NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[]) {
    try {
        const char* configpath = argc > 1 ? argv[1] : "config.json";
        AppWindow app(configpath, 1280, 720);
        app.MainLoop();
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
