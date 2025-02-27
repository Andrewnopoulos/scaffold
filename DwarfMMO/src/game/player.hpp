#pragma once

#include "game/entity.hpp"

class Player : public Entity {
public:
    Player(int x = 0, int y = 0);
    virtual ~Player() = default;
    
    // Override base entity methods
    void update(float deltaTime, World* world) override;
    void render(Renderer* renderer) override;
    
    // Player specific methods
    bool move(int dx, int dy, World* world = nullptr); // Changed to accept world parameter
    void setWorld(World* world) { m_world = world; }
    
private:
    World* m_world = nullptr;
    float m_moveTimer = 0.0f;
    float m_moveDelay = 0.1f; // Time between moves in seconds
};