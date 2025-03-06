#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "client/window.hpp"

// Forward declarations
class World;
class Entity;

class Renderer {
public:
    explicit Renderer(Window* window);
    ~Renderer();
    
    // Basic rendering operations
    void clear();
    void present();
    
    // Drawing methods
    void drawTile(int x, int y, char symbol, SDL_Color color);
    void drawCircle(int x, int y, int radius, SDL_Color color);
    void drawRect(int x, int y, int w, int h, SDL_Color color);
    void drawText(int x, int y, const std::string& text, SDL_Color color);
    
    // Getters
    SDL_Renderer* getSDLRenderer() const { return m_renderer; }
    
    // Disable copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
private:
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    
    // Tile size for grid-based rendering
    static constexpr int TILE_SIZE = 16;
};