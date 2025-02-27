#include <iostream>

#include "game/world.hpp"
#include "game/entity.hpp"
#include "client/renderer.hpp"

World::World(int width, int height)
    : m_width(width), m_height(height) {
    
    // Initialize tiles
    m_tiles.resize(width * height, Tile(TileType::EMPTY));
    
    // Generate a simple world
    generateSimpleWorld();
}

void World::update(float deltaTime) {
    // Update all entities
    for (auto& pair : m_entities) {
        pair.second->update(deltaTime, this);
    }
}

void World::render(Renderer* renderer) {
    // Render tiles
    // for (int y = 0; y < m_height; ++y) {
    //     for (int x = 0; x < m_width; ++x) {
    //         const Tile& tile = m_tiles[getIndex(x, y)];
    //         renderer->drawTile(x, y, tile.symbol, tile.color);
    //     }
    // }
    
    // Render entities
    if (m_entities.empty()) {
        // std::cout << "World::render - WARNING: No entities to render!" << std::endl;
    } else {
        for (const auto& pair : m_entities) {
            auto entity = pair.second;
            
            if (!entity->isVisible()) {
                std::cout << "  WARNING: Entity not visible! Setting to visible." << std::endl;
                entity->setVisible(true);
            }
            
            // Draw a slightly larger highlighted square for the entity to make it more visible
            SDL_Color highlightColor = entity->getColor();
            highlightColor.r = (highlightColor.r + 128) % 256;
            highlightColor.g = (highlightColor.g + 128) % 256;
            highlightColor.b = (highlightColor.b + 128) % 256;
            
            renderer->drawRect((entity->getX() * 16) - 2, 
                               (entity->getY() * 16) - 2, 
                               20, 20, highlightColor);
            
            // Now render the entity itself
            entity->render(renderer);
        }
    }
}

void World::setTile(int x, int y, TileType type) {
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
    return !isInBounds(x, y) || m_tiles[getIndex(x, y)].solid;
}

void World::addEntity(std::shared_ptr<Entity> entity) {
    uint32_t id = entity->getId();
    
    // Check if entity with this ID already exists
    if (m_entities.find(id) != m_entities.end()) {
        std::cerr << "Entity with ID " << id << " already exists!" << std::endl;
        // Try removing it and adding again
        m_entities.erase(id);
        std::cerr << "Removed old entity with ID " << id << " to replace it" << std::endl;
    }
    
    // Add entity to the map
    m_entities[id] = entity;
    std::cout << "Added entity ID: " << id << " to world. Total entities: " << m_entities.size() << std::endl;
}

void World::removeEntity(int id) {
    // Check if entity exists
    auto it = m_entities.find(id);
    if (it != m_entities.end()) {
        m_entities.erase(it);
        std::cout << "Removed entity ID: " << id << " from world. Total entities: " << m_entities.size() << std::endl;
    } else {
        std::cerr << "Entity with ID " << id << " not found in world." << std::endl;
    }
}

std::shared_ptr<Entity> World::getEntity(int id) {
    auto it = m_entities.find(id);
    if (it != m_entities.end()) {
        return it->second;
    }
    std::cout << "getEntity: Entity with ID " << id << " not found. Total entities: " << m_entities.size() << std::endl;
    return nullptr;
}

void World::generateSimpleWorld() {
    // Fill the world with empty space
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            setTile(x, y, TileType::EMPTY);
        }
    }
    
    // Create a simple room in the center
    int roomX = m_width / 2 - 15;
    int roomY = m_height / 2 - 10;
    int roomWidth = 30;
    int roomHeight = 20;
    
    // Floor
    for (int y = roomY; y < roomY + roomHeight; ++y) {
        for (int x = roomX; x < roomX + roomWidth; ++x) {
            setTile(x, y, TileType::FLOOR);
        }
    }
    
    // Walls
    for (int x = roomX; x < roomX + roomWidth; ++x) {
        setTile(x, roomY, TileType::WALL);
        setTile(x, roomY + roomHeight - 1, TileType::WALL);
    }
    
    for (int y = roomY; y < roomY + roomHeight; ++y) {
        setTile(roomX, y, TileType::WALL);
        setTile(roomX + roomWidth - 1, y, TileType::WALL);
    }
    
    // Door
    setTile(roomX + roomWidth / 2, roomY, TileType::FLOOR);
}