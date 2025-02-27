#include "game/entity.hpp"
#include "game/world.hpp"
#include "client/renderer.hpp"

Entity::Entity(int x, int y, char symbol)
    : m_x(x), m_y(y), m_symbol(symbol) {
}

void Entity::update(float deltaTime, World* world) {
    // Base entity doesn't do anything in update
}

void Entity::render(Renderer* renderer) {
    if (!m_isVisible) {
        return;
    }
    renderer->drawTile(m_x, m_y, m_symbol, m_color);
}

bool Entity::move(int dx, int dy, World* world) {
    if (!world) {
        // If no world is provided, just move without collision
        m_x += dx;
        m_y += dy;
        return true;
    }
    
    // Check for collision
    int newX = m_x + dx;
    int newY = m_y + dy;
    
    if (!world->isSolid(newX, newY)) {
        m_x = newX;
        m_y = newY;
        return true;
    }
    
    return false;
}