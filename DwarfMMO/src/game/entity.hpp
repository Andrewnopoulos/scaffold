#pragma once

#include <string>
#include <SDL2/SDL.h>

// Forward declarations
class World;
class Renderer;

class Entity {
public:
    Entity(int x = 0, int y = 0, char symbol = '?');
    virtual ~Entity() = default;
    
    // Core functionality
    virtual void update(float deltaTime, World* world);
    virtual void render(Renderer* renderer);
    
    // Movement
    virtual bool move(int dx, int dy, World* world = nullptr);
    
    // Getters and setters
    int getX() const { return m_x; }
    int getY() const { return m_y; }
    void setPosition(int x, int y) { m_x = x; m_y = y; }
    
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    char getSymbol() const { return m_symbol; }
    void setSymbol(char symbol) { m_symbol = symbol; }
    
    const SDL_Color& getColor() const { return m_color; }
    void setColor(const SDL_Color& color) { m_color = color; }
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    void setVisible(bool visible) { m_isVisible = visible; }
    bool isVisible() const { return m_isVisible; }
    
protected:
    int m_id = 0;
    int m_x = 0;
    int m_y = 0;
    char m_symbol = '?';
    SDL_Color m_color = {255, 255, 255, 255}; // Default: white
    std::string m_name = "Entity";
    bool m_isVisible = true;
};