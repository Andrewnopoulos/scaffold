#include "server/server.hpp"
#include "server/client_session.hpp"
#include <iostream>
#include <chrono>

Server::Server(boost::asio::io_context& ioContext, const ServerConfig& config)
    : m_ioContext(ioContext),
      m_config(config),
      m_running(false),
      m_acceptor(ioContext, tcp::endpoint(tcp::v4(), config.port)),
      m_nextPlayerId(1) {
    
    // Create the game world
    m_world = std::make_unique<World>(config.worldWidth, config.worldHeight);
    
    // Configure acceptor
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (m_running) {
        return;
    }
    
    m_running = true;
    
    // Start accepting connections
    startAccept();
    
    // Start game loop in a separate thread
    m_gameThread = std::thread(&Server::gameLoop, this);
    
    std::cout << "Server started on port " << m_config.port << std::endl;
}

void Server::stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    // Stop accepting connections
    m_acceptor.close();
    
    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& pair : m_clients) {
            pair.second->close();
        }
        m_clients.clear();
    }
    
    // Wait for game thread to finish
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
    
    std::cout << "Server stopped" << std::endl;
}

void Server::tick() {
    // Calculate delta time (fixed time step for now)
    float deltaTime = 1.0f / static_cast<float>(m_config.tickRate);
    
    // Update the world
    m_world->update(deltaTime);
    
    // Update all clients
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_clients) {
        pair.second->update(deltaTime);
    }
}

void Server::addPlayer(uint32_t playerId, std::shared_ptr<Player> player) {
    // Add player to the world
    m_world->addEntity(player);
    
    // Set initial position
    int centerX = m_config.worldWidth / 2;
    int centerY = m_config.worldHeight / 2;
    
    // Find an empty spot near the center
    for (int radius = 0; radius < 10; ++radius) {
        for (int y = centerY - radius; y <= centerY + radius; ++y) {
            for (int x = centerX - radius; x <= centerX + radius; ++x) {
                if (!m_world->isSolid(x, y)) {
                    player->setPosition(x, y);
                    
                    // Broadcast new player to all clients
                    broadcastPlayerPosition(playerId, x, y);
                    return;
                }
            }
        }
    }
    
    // If no empty spot found, just place at center
    player->setPosition(centerX, centerY);
    broadcastPlayerPosition(playerId, centerX, centerY);
}

void Server::removePlayer(uint32_t playerId) {
    // Remove player from the world
    m_world->removeEntity(playerId);
    
    // Broadcast player disconnect to all clients
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    // Get player name before removing the client
    std::string playerName = "Unknown";
    auto clientIt = m_clients.find(playerId);
    if (clientIt != m_clients.end() && clientIt->second->getPlayer()) {
        playerName = clientIt->second->getPlayer()->getName();
    }
    
    // Create disconnect packet
    DisconnectPacket packet(playerName);
    
    // Send to all clients except the disconnecting one
    for (auto& pair : m_clients) {
        if (pair.first != playerId) {
            pair.second->sendPacket(packet);
        }
    }
    
    // Remove client session
    m_clients.erase(playerId);
    
    std::cout << "Player removed: " << playerName << " (ID: " << playerId << ")" << std::endl;
}

uint32_t Server::getNextPlayerId() {
    return m_nextPlayerId++;
}

World* Server::getWorld() {
    return m_world.get();
}

const ServerConfig& Server::getConfig() const {
    return m_config;
}

void Server::broadcastPlayerPosition(uint32_t playerId, int x, int y) {
    // Create player position packet
    PlayerPositionPacket packet(playerId, x, y);
    
    // Send to ALL clients, including the originating player
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_clients) {
        // Send to everyone to ensure consistency
        pair.second->sendPacket(packet);
    }
}

void Server::broadcastWorldModification(int x, int y, uint8_t tileType) {
    // Create world modification packet
    WorldModificationPacket packet(x, y, tileType);
    
    // Send to all clients
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& pair : m_clients) {
        pair.second->sendPacket(packet);
    }
}

void Server::startAccept() {
    if (!m_running) {
        return;
    }
    
    // Create a new session
    auto session = std::make_shared<ClientSession>(m_ioContext, this);
    
    // Accept a new connection
    m_acceptor.async_accept(
        session->getSocket(),
        [this, session](const boost::system::error_code& error) {
            handleAccept(session, error);
        }
    );
}

void Server::handleAccept(ClientSessionPtr session, const boost::system::error_code& error) {
    if (error) {
        std::cerr << "Accept error: " << error.message() << std::endl;
    } else {
        // Check if we've reached the maximum number of clients
        size_t clientCount;
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            clientCount = m_clients.size();
        }
        
        if (clientCount >= m_config.maxClients) {
            std::cerr << "Rejecting connection: maximum clients reached" << std::endl;
            session->close();
        } else {
            // Start the session
            session->start();
            
            std::lock_guard<std::mutex> lock(m_clientsMutex);
        }
    }
    
    // Continue accepting connections
    startAccept();
}

void Server::gameLoop() {
    using clock = std::chrono::high_resolution_clock;
    
    auto lastTick = clock::now();
    auto tickDuration = std::chrono::milliseconds(1000 / m_config.tickRate);
    
    while (m_running) {
        auto currentTime = clock::now();
        auto elapsedTime = currentTime - lastTick;
        
        if (elapsedTime >= tickDuration) {
            // Run a game tick
            tick();
            
            // Calculate how many ticks we should have had
            int64_t ticksToRun = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() / 
                                 (1000 / m_config.tickRate);
            
            // Run additional ticks if we're falling behind
            for (int64_t i = 1; i < ticksToRun && i < m_config.maxUpdatesPerTick; ++i) {
                tick();
            }
            
            lastTick = currentTime;
        }
        
        // Sleep to avoid using 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}