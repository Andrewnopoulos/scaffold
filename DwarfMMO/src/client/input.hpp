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
    
    // Process input events, return false if the game should exit
    bool processInput(Player* player, World* world = nullptr, NetworkClient* network = nullptr);
    
    // Check if a key is currently pressed
    bool isKeyPressed(SDL_Scancode key) const;
    
private:
    std::unordered_map<SDL_Scancode, bool> m_keyStates;
};