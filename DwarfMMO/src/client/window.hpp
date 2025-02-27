#pragma once

#include <string>
#include <SDL2/SDL.h>

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();
    
    // Getters
    SDL_Window* getSDLWindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    // Disable copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
private:
    SDL_Window* m_window;
    int m_width;
    int m_height;
};