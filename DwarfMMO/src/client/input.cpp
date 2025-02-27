#include "client/input.hpp"
#include "game/player.hpp"
#include "game/world.hpp"
#include "network/client.hpp"

bool InputHandler::processInput(Player* player, World* world, NetworkClient* network) {
    SDL_Event event;
    
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
                
                // Handle world modification keys (1, 2, 3 for different tile types)
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
                            // Update local world immediately for responsiveness
                            world->setTile(targetX, targetY, newType);
                            
                            // Send world modification to server
                            WorldModificationPacket packet(targetX, targetY, static_cast<uint8_t>(newType));
                            network->sendPacket(packet);
                        }
                    }
                }
                break;
                
            case SDL_KEYUP:
                m_keyStates[event.key.keysym.scancode] = false;
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