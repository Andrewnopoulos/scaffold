#include "game/world.hpp"
#include "game/entity.hpp"
#include "game/player.hpp"
#include <algorithm>
#include <iostream>

World::World(int width, int height)
    : m_width(width), m_height(height) {
    
    // Initialize tiles
    m_tiles.resize(width * height, Tile(TileType::EMPTY));
    
    // Generate the world
    generateWorld();
    
    std::cout << "World created with size " << width << "x" << height << std::endl;
}

void World::update(float deltaTime) {
    // Update all entities
    std::lock_guard<std::mutex> lockEntities(m_entityMutex);
    for (auto& pair : m_entities) {
        pair.second->update(deltaTime, this);
    }
}

void World::setTile(int x, int y, TileType type) {
    std::lock_guard<std::mutex> lock(m_worldMutex);
    if (isInBounds(x, y)) {
        m_tiles[getIndex(x, y)] = Tile(type);
    }
}

Tile World::getTile(int x, int y) const {
    if (isInBounds(x, y)) {
        return m_tiles[getIndex(x, y)];
    }
    return Tile(TileType::WALL); // Default to wall for out-of-bounds
}

bool World::isSolid(int x, int y) const {
    if (!isInBounds(x, y)) {
        return true; // Out of bounds is solid
    }
    return m_tiles[getIndex(x, y)].solid;
}

void World::addEntity(std::shared_ptr<Entity> entity) {
    std::lock_guard<std::mutex> lock(m_entityMutex);
    m_entities[entity->getId()] = entity;
}

void World::removeEntity(int id) {
    std::lock_guard<std::mutex> lock(m_entityMutex);
    m_entities.erase(id);
}

std::shared_ptr<Entity> World::getEntity(int id) {
    std::lock_guard<std::mutex> lock(m_entityMutex);
    auto it = m_entities.find(id);
    if (it != m_entities.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Player>> World::getPlayersInRange(int x, int y, int range) {
    std::vector<std::shared_ptr<Player>> players;
    std::lock_guard<std::mutex> lock(m_entityMutex);
    
    for (auto& pair : m_entities) {
        auto entity = pair.second;
        auto player = std::dynamic_pointer_cast<Player>(entity);
        
        if (player) {
            int dx = player->getX() - x;
            int dy = player->getY() - y;
            int distanceSquared = dx * dx + dy * dy;
            
            if (distanceSquared <= range * range) {
                players.push_back(player);
            }
        }
    }
    
    return players;
}

void World::generateWorld() {
    // Fill the world with empty space
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            setTile(x, y, TileType::EMPTY);
        }
    }
    
    // Create a central area with floor
    int centerX = m_width / 2;
    int centerY = m_height / 2;
    int roomSize = std::min(m_width, m_height) / 4;
    
    for (int y = centerY - roomSize; y <= centerY + roomSize; ++y) {
        for (int x = centerX - roomSize; x <= centerX + roomSize; ++x) {
            if (isInBounds(x, y)) {
                setTile(x, y, TileType::FLOOR);
            }
        }
    }
    
    // Add some walls around the central area
    for (int y = centerY - roomSize - 1; y <= centerY + roomSize + 1; ++y) {
        for (int x = centerX - roomSize - 1; x <= centerX + roomSize + 1; ++x) {
            if (isInBounds(x, y)) {
                if (x == centerX - roomSize - 1 || x == centerX + roomSize + 1 ||
                    y == centerY - roomSize - 1 || y == centerY + roomSize + 1) {
                    setTile(x, y, TileType::WALL);
                }
            }
        }
    }
    
    // Create openings in the walls
    setTile(centerX, centerY - roomSize - 1, TileType::FLOOR); // North
    setTile(centerX, centerY + roomSize + 1, TileType::FLOOR); // South
    setTile(centerX - roomSize - 1, centerY, TileType::FLOOR); // West
    setTile(centerX + roomSize + 1, centerY, TileType::FLOOR); // East
    
    // Add some random walls inside the room for obstacles
    for (int i = 0; i < roomSize * 2; ++i) {
        int x = centerX - roomSize + (rand() % (roomSize * 2));
        int y = centerY - roomSize + (rand() % (roomSize * 2));
        
        if (isInBounds(x, y)) {
            // Don't block the center or the openings
            if (abs(x - centerX) > 2 || abs(y - centerY) > 2) {
                setTile(x, y, TileType::WALL);
            }
        }
    }
}