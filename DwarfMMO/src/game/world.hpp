#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <SDL2/SDL.h>

// Forward declarations
class Renderer;
class Entity;

// Tile type enum
enum class TileType {
    EMPTY,
    FLOOR,
    WALL,
    GREEN_WALL
};

// Tile structure
struct Tile {
    TileType type;
    char symbol;
    SDL_Color color;
    bool solid;
    
    Tile(TileType t = TileType::EMPTY) : type(t) {
        switch (t) {
            case TileType::EMPTY:
                symbol = ' ';
                color = {0, 0, 0, 255};
                solid = false;
                break;
            case TileType::FLOOR:
                symbol = '.';
                color = {100, 100, 100, 255};
                solid = false;
                break;
            case TileType::WALL:
                symbol = '#';
                color = {150, 150, 150, 255};
                solid = true;
                break;
            case TileType::GREEN_WALL:
                symbol = '#';
                color = {0, 200, 0, 255};
                solid = true;
                break;
        }
    }
};

class World {
public:
    World(int width = 100, int height = 100);
    ~World() = default;
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    // World modification
    void setTile(int x, int y, TileType type);
    Tile getTile(int x, int y) const;
    bool isSolid(int x, int y) const;
    
    // Entity management
    void addEntity(std::shared_ptr<Entity> entity);
    void removeEntity(int id);
    std::shared_ptr<Entity> getEntity(int id);
    
    // Getters
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    // Get all entities
    const std::unordered_map<int, std::shared_ptr<Entity>>& getEntities() const { return m_entities; }
    
private:
    int m_width;
    int m_height;
    std::vector<Tile> m_tiles;
    std::unordered_map<int, std::shared_ptr<Entity>> m_entities;
    int m_nextEntityId = 1;
    
    // Helper for index calculation
    inline int getIndex(int x, int y) const {
        return y * m_width + x;
    }
    
    // Check if coordinates are within bounds
    inline bool isInBounds(int x, int y) const {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }
    
    // Initialize the world with a simple room
    void generateSimpleWorld();
};