#include "app/Application.hpp"
#include "core/EngineConstants.hpp"
#include <iostream>

int main() {
    try {
        Application app(
            EngineConstants::Window::DEFAULT_WIDTH,
            EngineConstants::Window::DEFAULT_HEIGHT,
            EngineConstants::Window::TITLE
        );
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[Fatal Error]: " << e.what() << std::endl;
        return 1;
    }
}
