#include "game/player.hpp"
#include "game/world.hpp"
#include "client/renderer.hpp"
#include <random>
#include <ctime>

Player::Player(int x, int y)
    : Entity(x, y, '@') {

    // Set up random number generator
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(0, 255);
    
    // Set player color to a random color
    m_color = {
        static_cast<unsigned char>(dist(rng)),  // Random red
        static_cast<unsigned char>(dist(rng)),  // Random green
        static_cast<unsigned char>(dist(rng)),  // Random blue
        255  // Keep full opacity
    };
    
    m_name = "Player";
}

void Player::update(float deltaTime, World* world) {
    m_world = world;
    m_moveTimer -= deltaTime;
}

void Player::render(Renderer* renderer) {
    if (!m_isVisible) {
        return;
    }
    
    // Render player as a colored circle
    int centerX = m_x * 16 + 8;  // Center of the tile
    int centerY = m_y * 16 + 8;
    int radius = 7;  // Slightly smaller than tile size
    
    // Draw the player circle
    renderer->drawCircle(centerX, centerY, radius, m_color);
    
    // Draw player name above the player
    SDL_Color textColor = {255, 255, 255, 255};  // White text for better visibility
    renderer->drawText(centerX, centerY - radius - 10, m_name, textColor);
}

bool Player::move(int dx, int dy, World* world) {
    // Use provided world parameter if available, otherwise use stored world
    World* targetWorld = world ? world : m_world;
    
    if (m_moveTimer <= 0.0f) {
        if (Entity::move(dx, dy, targetWorld)) {
            m_moveTimer = m_moveDelay;
            return true;
        }
    }
    return false;
}