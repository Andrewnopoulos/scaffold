#include <iostream>
#include <memory>
#include <string>

#include "client/window.hpp"
#include "client/renderer.hpp"
#include "client/input.hpp"
#include "game/world.hpp"
#include "game/player.hpp"
#include "network/client.hpp"
#include "network/packet.hpp"

// Server connection settings
const std::string SERVER_HOST = "127.0.0.1"; // localhost by default
const uint16_t SERVER_PORT = 7777;           // default server port


// Add debug function to print all entities in the world
void printWorldEntities(const World* world) {
    std::cout << "=== World Entities ===" << std::endl;
    for (const auto& pair : world->getEntities()) {
        auto entity = pair.second;
        std::cout << "Entity ID: " << entity->getId() 
                  << ", Name: " << entity->getName()
                  << ", Position: (" << entity->getX() << "," << entity->getY() << ")" 
                  << std::endl;
    }
    std::cout << "=====================" << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        std::string serverHost = SERVER_HOST;
        uint16_t serverPort = SERVER_PORT;
        
        // Check command line arguments for server host and port
        if (argc > 1) {
            serverHost = argv[1];
        }
        if (argc > 2) {
            serverPort = static_cast<uint16_t>(std::stoi(argv[2]));
        }
        
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
            return 1;
        }
        
        // Create window
        std::unique_ptr<Window> window = std::make_unique<Window>("Dwarf MMO", 800, 600);
        
        // Create renderer
        std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>(window.get());
        
        // Create input handler
        std::unique_ptr<InputHandler> input = std::make_unique<InputHandler>();
        
        // Create network client
        std::unique_ptr<NetworkClient> network = std::make_unique<NetworkClient>();
        
        // Create world
        std::unique_ptr<World> world = std::make_unique<World>();
        
        // Create player
        std::unique_ptr<Player> player = std::make_unique<Player>();
        
        // Add player to the world
        player->setVisible(true);
        
        // Connect to server
        std::cout << "Connecting to server at " << serverHost << ":" << serverPort << "..." << std::endl;
        if (!network->connect(serverHost, serverPort)) {
            std::cerr << "Failed to connect to server!" << std::endl;
            SDL_Quit();
            return 1;
        }
        std::cout << "Connected to server!" << std::endl;
        
        // Set up player name
        std::string playerName = "Player";
        std::cout << "Enter your player name: ";
        std::getline(std::cin, playerName);
        if (playerName.empty()) {
            playerName = "Player";
        }
        
        // Send connection request
        ConnectRequestPacket connectPacket(playerName);
        network->sendPacket(connectPacket);
        
        // Set up packet handlers
        network->setPacketHandler<ConnectAcceptPacket>([&](const ConnectAcceptPacket& packet) {
            uint32_t playerId = packet.getPlayerId();
            player->setId(playerId);
        });
        
        network->setPacketHandler<PlayerPositionPacket>([&](const PlayerPositionPacket& packet) {
            uint32_t playerId = packet.getPlayerId();
            if (playerId != player->getId()) {
                // Update position of other player
                auto otherPlayer = world->getEntity(playerId);
                if (!otherPlayer) {
                    // Create new player entity with an explicit shared_ptr 
                    std::shared_ptr<Player> newPlayer = std::make_shared<Player>(packet.getX(), packet.getY());
                    newPlayer->setId(playerId);
                    newPlayer->setName("Player " + std::to_string(playerId)); // Default name until we get appearance
                    newPlayer->setVisible(true); // Ensure visibility is set
                    
                    // Try setting a different color to make it more obvious
                    SDL_Color otherColor = {0, 255, 0, 255}; // Make it bright green
                    newPlayer->setColor(otherColor);
                    
                    // Insert it into the world
                    world->addEntity(newPlayer);
                    
                    // When we receive a new player, force an appearance packet for our own player
                    SDL_Color myColor = player->getColor();
                    PlayerAppearancePacket appearanceUpdate(
                        player->getId(), 
                        player->getSymbol(),
                        myColor.r, myColor.g, myColor.b,
                        playerName
                    );
                    network->sendPacket(appearanceUpdate);
                    
                    // Also force a position update to the server for our player
                    PlayerPositionPacket posUpdate(player->getId(), player->getX(), player->getY());
                    network->sendPacket(posUpdate);
                } else {
                    // Update existing player position
                    otherPlayer->setPosition(packet.getX(), packet.getY());
                    otherPlayer->setVisible(true); // Ensure visibility is set
                }
            }
        });

        network->setPacketHandler<PlayerAppearancePacket>([&](const PlayerAppearancePacket& packet) {
            uint32_t playerId = packet.getPlayerId();
            if (playerId != player->getId()) {
                auto otherPlayer = world->getEntity(playerId);
                if (!otherPlayer) {
                    // Create new player entity
                    std::shared_ptr<Player> newPlayer = std::make_shared<Player>();
                    newPlayer->setId(playerId);
                    newPlayer->setPosition(0, 0); // Default position until we get position update
                    newPlayer->setVisible(true); // Ensure it's visible!
                    
                    // Insert it into the world
                    world->addEntity(newPlayer);
                    otherPlayer = newPlayer;
                }
                
                // Set player appearance
                otherPlayer->setSymbol(packet.getSymbol());
                SDL_Color color = {
                    packet.getColorR(),
                    packet.getColorG(),
                    packet.getColorB(),
                    255 // Alpha is always 255
                };
                otherPlayer->setColor(color);
                otherPlayer->setName(packet.getName());
                otherPlayer->setVisible(true); // Ensure visibility is set
            }
        });
        
        network->setPacketHandler<PlayerListPacket>([&](const PlayerListPacket& packet) {
            for (const auto& playerInfo : packet.getPlayers()) {
                uint32_t playerId = playerInfo.id;
                
                // Skip our own player
                if (playerId == player->getId()) {
                    continue;
                }
                auto otherPlayer = world->getEntity(playerId);
                if (!otherPlayer) {
                    // Create new player entity with an explicit shared_ptr
                    std::shared_ptr<Player> newPlayer = std::make_shared<Player>(playerInfo.x, playerInfo.y);
                    newPlayer->setId(playerId);
                    newPlayer->setName(playerInfo.name);
                    newPlayer->setVisible(true); // Ensure visibility is set
                    
                    // Set a distinctive color
                    SDL_Color otherColor = {0, 0, 255, 255}; // Make it blue
                    newPlayer->setColor(otherColor);
                    
                    world->addEntity(newPlayer);
                } else {
                    otherPlayer->setVisible(true); // Ensure visibility is set
                    otherPlayer->setPosition(playerInfo.x, playerInfo.y);
                    otherPlayer->setName(playerInfo.name);
                }
            }
        });
        
        network->setPacketHandler<WorldModificationPacket>([&](const WorldModificationPacket& packet) {
            // Update local world tile
            world->setTile(packet.getX(), packet.getY(), static_cast<TileType>(packet.getTileType()));
        });
        
        network->setPacketHandler<WorldChunkPacket>([&](const WorldChunkPacket& packet) {
            int chunkX = packet.getX();
            int chunkY = packet.getY();
            int width = packet.getWidth();
            int height = packet.getHeight();
            
            const auto& tileData = packet.getTileData();
            size_t index = 0;
            
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (index < tileData.size()) {
                        TileType type = static_cast<TileType>(tileData[index++]);
                        world->setTile(chunkX + x, chunkY + y, type);
                    }
                }
            }
        });

        network->setPacketHandler<DisconnectPacket>([&](const DisconnectPacket& packet) {
            // The server sends player name in the reason field when a player disconnects
            std::string playerName = packet.getReason();
            
            // Iterate through entities to find and remove the disconnected player
            for (auto it = world->getEntities().begin(); it != world->getEntities().end(); ) {
                auto entity = it->second;
                if (entity->getName() == playerName) {
                    world->removeEntity(entity->getId());
                    break;
                } else {
                    ++it;
                }
            }
        });

        // Before the game loop, add a signal handler or similar cleanup function
        auto cleanup = [&]() {
            if (network->isConnected()) {
                // Send disconnect packet
                DisconnectPacket disconnectPacket("Client disconnected");
                network->sendPacket(disconnectPacket);
                network->disconnect();
            }
            SDL_Quit();
        };

        // Game loop
        bool running = true;
        Uint32 lastTime = SDL_GetTicks();
        static Uint32 lastDebugTime = 0;
        
        while (running) {
            // Calculate delta time
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            // if (currentTime - lastDebugTime > 5000) { // Every 5 seconds
            //     lastDebugTime = currentTime;
            //     printWorldEntities(world.get());
                
            //     std::cout << "DEBUG: Sending test position update for local player" << std::endl;
            //     // Force a position update of our own player
            //     PlayerPositionPacket posUpdate(player->getId(), player->getX(), player->getY());
            //     network->sendPacket(posUpdate);
                
            //     // Force an appearance update for our player too
            //     SDL_Color myColor = player->getColor();
            //     PlayerAppearancePacket appearanceUpdate(
            //         player->getId(),
            //         player->getSymbol(),
            //         myColor.r, myColor.g, myColor.b,
            //         playerName
            //     );
            //     network->sendPacket(appearanceUpdate);
            // }
            
            // Handle input
            running = input->processInput(player.get(), world.get(), network.get());
            
            // Update network (process received packets)
            network->update();
            
            // Send player position to server if player moved
            static int lastX = -1, lastY = -1;
            if (player->getX() != lastX || player->getY() != lastY) {
                lastX = player->getX();
                lastY = player->getY();
                
                PlayerPositionPacket posPacket(player->getId(), lastX, lastY);
                network->sendPacket(posPacket);
            }
            
            // Update game logic
            world->update(deltaTime);
            player->update(deltaTime, world.get());
            
            // Render
            renderer->clear();
            
            // Only call world->render if player is in the world
            world->render(renderer.get());
            
            // Always render the local player separately since it's not in the world collection
            // (We're using a unique_ptr for the local player, not compatible with the shared_ptr in world)
            player->render(renderer.get());
            
            renderer->present();
            
            // Cap framerate to ~60 FPS
            if (deltaTime < 1.0f/60.0f) {
                SDL_Delay((1.0f/60.0f - deltaTime) * 1000.0f);
            }
        }
        
        // Clean disconnect from server
        cleanup();

        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        SDL_Quit();
        return 1;
    }
}