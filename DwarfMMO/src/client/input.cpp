#include "client/input.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "network/client.hpp"
#include <iostream>

// Helper to convert screen coordinates to tile coordinates
void InputHandler::screenToTile(int screenX, int screenY, int& tileX, int& tileY) const {
    // Assuming 16x16 tile size
    tileX = screenX / 16;
    tileY = screenY / 16;
}

bool InputHandler::processInput(Player* player, World* world, NetworkClient* network) {
    SDL_Event event;
    
    // Update mouse state
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    m_mouseX = mouseX;
    m_mouseY = mouseY;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;
                
            case SDL_KEYDOWN:
                m_keyStates[event.key.keysym.scancode] = true;
                
                // Handle specific key presses
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    return false; // Exit game on ESC
                }
                
                // Toggle wall placement mode with 'G' key
                if (event.key.keysym.scancode == SDL_SCANCODE_G) {
                    toggleWallPlacement();
                    std::cout << "Wall placement mode " << (m_placingWalls ? "enabled" : "disabled") << std::endl;
                }
                
                // Handle world modification keys (1, 2, 3, 4 for different tile types)
                if (world && network && player) {
                    int playerX = player->getX();
                    int playerY = player->getY();
                    
                    TileType newType = TileType::EMPTY;
                    bool modifyWorld = false;
                    
                    if (event.key.keysym.scancode == SDL_SCANCODE_1) {
                        newType = TileType::EMPTY;
                        modifyWorld = true;
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_2) {
                        newType = TileType::FLOOR;
                        modifyWorld = true;
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_3) {
                        newType = TileType::WALL;
                        modifyWorld = true;
                    } else if (event.key.keysym.scancode == SDL_SCANCODE_4) {
                        newType = TileType::GREEN_WALL;
                        modifyWorld = true;
                    }
                    
                    // Handle directional tile modifications with Shift+Arrow keys
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        int targetX = playerX;
                        int targetY = playerY;
                        
                        if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
                            targetY--;
                            modifyWorld = true;
                        } else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                            targetY++;
                            modifyWorld = true;
                        } else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                            targetX--;
                            modifyWorld = true;
                        } else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                            targetX++;
                            modifyWorld = true;
                        }
                        
                        if (modifyWorld) {
                            // Calculate distance from player
                            int dx = targetX - playerX;
                            int dy = targetY - playerY;
                            float distance = std::sqrt(dx * dx + dy * dy);
                            
                            // Check if within interact range
                            if (distance <= PLAYER_INTERACT_RANGE) {
                                // Update local world immediately for responsiveness
                                world->setTile(targetX, targetY, newType);
                                
                                // Send world modification to server
                                WorldModificationPacket packet(targetX, targetY, static_cast<uint8_t>(newType));
                                network->sendPacket(packet);
                            } else {
                                std::cout << "Cannot place tile: too far from player (distance: " 
                                          << distance << " > " << PLAYER_INTERACT_RANGE << ")" << std::endl;
                            }
                        }
                    }
                }
                break;
                
            case SDL_KEYUP:
                m_keyStates[event.key.keysym.scancode] = false;
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (m_placingWalls && world && network && player && event.button.button == SDL_BUTTON_LEFT) {
                    int tileX, tileY;
                    screenToTile(m_mouseX, m_mouseY, tileX, tileY);
                    
                    // Calculate distance from player
                    int playerX = player->getX();
                    int playerY = player->getY();
                    int dx = tileX - playerX;
                    int dy = tileY - playerY;
                    float distance = std::sqrt(dx * dx + dy * dy);
                    
                    // Check if within interact range
                    if (distance <= PLAYER_INTERACT_RANGE) {
                        // Place a green wall at the clicked location
                        world->setTile(tileX, tileY, TileType::GREEN_WALL);
                        
                        // Send world modification to server
                        WorldModificationPacket packet(tileX, tileY, static_cast<uint8_t>(TileType::GREEN_WALL));
                        network->sendPacket(packet);
                    } else {
                        std::cout << "Cannot place wall: too far from player (distance: " 
                                  << distance << " > " << PLAYER_INTERACT_RANGE << ")" << std::endl;
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // Handle player movement
    if (player) {
        int dx = 0, dy = 0;
        
        if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_UP)) {
            dy = -1;
        }
        if (isKeyPressed(SDL_SCANCODE_S) || isKeyPressed(SDL_SCANCODE_DOWN)) {
            dy = 1;
        }
        if (isKeyPressed(SDL_SCANCODE_A) || isKeyPressed(SDL_SCANCODE_LEFT)) {
            dx = -1;
        }
        if (isKeyPressed(SDL_SCANCODE_D) || isKeyPressed(SDL_SCANCODE_RIGHT)) {
            dx = 1;
        }
        
        if (dx != 0 || dy != 0) {
            player->move(dx, dy, world);
        }
    }
    
    // Return true to continue running
    return true;
}

bool InputHandler::isKeyPressed(SDL_Scancode key) const {
    auto it = m_keyStates.find(key);
    return (it != m_keyStates.end() && it->second);
}