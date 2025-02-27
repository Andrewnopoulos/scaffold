#include "client/window.hpp"
#include <stdexcept>

Window::Window(const std::string& title, int width, int height)
    : m_width(width), m_height(height) {
    
    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    
    if (!m_window) {
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }
}

Window::~Window() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
}