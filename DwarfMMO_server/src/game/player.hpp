#pragma once

#include "game/entity.hpp"
#include <chrono>

class Player : public Entity {
public:
    Player(int x = 0, int y = 0);
    virtual ~Player() = default;
    
    // Override base entity methods
    void update(float deltaTime, World* world) override;
    bool move(int dx, int dy, World* world = nullptr) override;
    
    // Player specific methods
    bool canMove() const;
    void resetMoveTimer();
    
    // Player state
    bool isOnline() const { return m_online; }
    void setOnline(bool online) { m_online = online; }
    
    uint64_t getLastActivity() const { return m_lastActivity; }
    void updateActivity();
    
private:
    bool m_online = false;
    float m_moveTimer = 0.0f;
    float m_moveDelay = 0.1f; // Time between moves in seconds
    uint64_t m_lastActivity = 0; // Unix timestamp of last activity
};