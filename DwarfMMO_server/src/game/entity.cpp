#include "game/entity.hpp"
#include "game/world.hpp"

Entity::Entity(int x, int y, char symbol)
    : m_x(x), m_y(y), m_symbol(symbol) {
}

void Entity::update(float deltaTime, World* world) {
    // Base entity doesn't do anything in update
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