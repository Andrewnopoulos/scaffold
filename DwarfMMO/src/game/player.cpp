#include "game/player.hpp"
#include "game/world.hpp"
#include "client/renderer.hpp"

Player::Player(int x, int y)
    : Entity(x, y, '@') {
    
    // Set player color to yellow
    m_color = {255, 255, 0, 255};
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
    // Call base class render
    Entity::render(renderer);
    
    // Additional player-specific rendering
    // For example, you might want to highlight the player's position
    SDL_Color highlightColor = {255, 255, 0, 64}; // Semi-transparent yellow
    renderer->drawRect(m_x * 16 - 2, m_y * 16 - 2, 20, 20, highlightColor);
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