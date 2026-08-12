#include "app/Application.hpp"
#include <iostream>

int main() {
    try {
        Application app(1280, 720, "Vulkan Modular Terrain Engine | Real-time Compute Generation & Dynamic LOD");
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[Fatal Error]: " << e.what() << std::endl;
        return 1;
    }
}
