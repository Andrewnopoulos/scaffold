#include "game/player.hpp"
#include "game/world.hpp"

Player::Player(int x, int y)
    : Entity(x, y, '@') {
    
    // Set player color to yellow
    m_color = {255, 255, 0, 255};
    m_name = "Player";
    
    // Initialize last activity timestamp
    updateActivity();
}

void Player::update(float deltaTime, World* world) {
    // Update move timer
    if (m_moveTimer > 0.0f) {
        m_moveTimer -= deltaTime;
    }
}

bool Player::move(int dx, int dy, World* world) {
    if (canMove()) {
        if (Entity::move(dx, dy, world)) {
            resetMoveTimer();
            updateActivity();
            return true;
        }
    }
    return false;
}

bool Player::canMove() const {
    return m_moveTimer <= 0.0f;
}

void Player::resetMoveTimer() {
    m_moveTimer = m_moveDelay;
}

void Player::updateActivity() {
    // Update the timestamp of the player's last activity
    auto now = std::chrono::system_clock::now();
    m_lastActivity = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}