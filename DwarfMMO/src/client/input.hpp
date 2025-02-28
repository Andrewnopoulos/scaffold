#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include "network/client.hpp"

// Forward declaration
class Player;
class World;

class InputHandler {
public:
    InputHandler() = default;
    ~InputHandler() = default;
    
    // Maximum distance a player can interact with the world
    // Must match server configuration (ServerConfig::playerInteractRange)
    static constexpr float PLAYER_INTERACT_RANGE = 5.0f;
    
    // Process input events, return false if the game should exit
    bool processInput(Player* player, World* world = nullptr, NetworkClient* network = nullptr);
    
    // Check if a key is currently pressed
    bool isKeyPressed(SDL_Scancode key) const;
    
    // Wall placement functions
    bool isPlacingWalls() const { return m_placingWalls; }
    void toggleWallPlacement() { m_placingWalls = !m_placingWalls; }
    
    // Mouse position getters
    int getMouseX() const { return m_mouseX; }
    int getMouseY() const { return m_mouseY; }
    
    // Convert screen coordinates to tile coordinates
    void screenToTile(int screenX, int screenY, int& tileX, int& tileY) const;
    
private:
    std::unordered_map<SDL_Scancode, bool> m_keyStates;
    bool m_placingWalls = false;
    int m_mouseX = 0;
    int m_mouseY = 0;
};