#include "client/renderer.hpp"
#include <stdexcept>

Renderer::Renderer(Window* window) {
    m_renderer = SDL_CreateRenderer(
        window->getSDLWindow(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!m_renderer) {
        throw std::runtime_error("Failed to create renderer: " + std::string(SDL_GetError()));
    }
    
    // Set initial render color
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    
    // For a full implementation, you'd initialize SDL_ttf and load a font here
}

Renderer::~Renderer() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
    }
    
    // For a full implementation, you'd clean up SDL_ttf resources here
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::present() {
    SDL_RenderPresent(m_renderer);
}

void Renderer::drawTile(int x, int y, char symbol, SDL_Color color) {
    // For now, just draw a colored rectangle
    // In a full implementation, you'd render the character using SDL_ttf
    SDL_Rect rect = {
        x * TILE_SIZE,
        y * TILE_SIZE,
        TILE_SIZE,
        TILE_SIZE
    };
    
    // Draw the tile with requested color
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
    
    // Add a border to make it more visible
    SDL_SetRenderDrawColor(m_renderer, 255 - color.r, 255 - color.g, 255 - color.b, color.a);
    SDL_RenderDrawRect(m_renderer, &rect);
    
    // Drawing the actual character would require SDL_ttf
}

void Renderer::drawRect(int x, int y, int w, int h, SDL_Color color) {
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(m_renderer, &rect);
}

void Renderer::drawText(int x, int y, const std::string& text, SDL_Color color) {
    // This would require SDL_ttf in a full implementation
    // For now, this is just a placeholder
}