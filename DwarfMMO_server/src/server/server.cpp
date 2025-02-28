#include "server/server.hpp"
#include "server/client_session.hpp"
#include <iostream>
#include <chrono>
#include <future>

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
    
    std::cout << "Stopping server..." << std::endl;
    m_running = false;
    
    // Stop accepting connections
    try {
        m_acceptor.close();
        std::cout << "Stopped accepting new connections" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error closing acceptor: " << e.what() << std::endl;
    }
    
    // Close all client connections with a timeout mechanism
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        size_t clientCount = m_clients.size();
        std::cout << "Closing " << clientCount << " client connections..." << std::endl;
        
        // Make a copy of the clients to avoid modification during iteration
        auto clientsCopy = m_clients;
        m_clients.clear();  // Clear the main map immediately
        
        // FIRST send shutdown notifications to all clients
        for (auto& pair : clientsCopy) {
            try {
                std::cout << "Sending shutdown notification to client ID: " << pair.first << std::endl;
                pair.second->sendShutdownNotification();
            } catch (const std::exception& e) {
                std::cerr << "Error sending shutdown notification to client " << pair.first << ": " << e.what() << std::endl;
            }
        }
        
        // Give clients a little time to process the notification
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // THEN close all the connections
        for (auto& pair : clientsCopy) {
            try {
                std::cout << "Closing client ID: " << pair.first << std::endl;
                pair.second->close();
            } catch (const std::exception& e) {
                std::cerr << "Error closing client " << pair.first << ": " << e.what() << std::endl;
            }
        }
        
        clientsCopy.clear();  // Release all shared_ptrs
        std::cout << "All client connections closed" << std::endl;
    }
    
    // Important: Wait for the game thread to finish FIRST with a timeout
    // to prevent deadlocks with client session operations
    if (m_gameThread.joinable()) {
        std::cout << "Waiting for game thread to finish..." << std::endl;
        
        // Use a timeout mechanism to prevent infinite waiting
        auto future = std::async(std::launch::async, [this]() {
            if (m_gameThread.joinable()) {
                m_gameThread.join();
                return true;
            }
            return false;
        });
        
        // Wait up to 2 seconds for the thread to join
        if (future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            std::cerr << "Game thread join timed out, server may not shut down cleanly" << std::endl;
            // We can't safely detach or terminate the thread here
            // Just continue with server shutdown
        } else {
            std::cout << "Game thread finished" << std::endl;
        }
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
        // Make sure to send to ALL clients, even the one that made the modification
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
        // Don't report errors when server is shutting down
        if (error != boost::asio::error::operation_aborted || m_running) {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }
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
    
    // Only continue accepting connections if server is still running
    if (m_running) {
        startAccept();
    }
}

void Server::gameLoop() {
    using clock = std::chrono::high_resolution_clock;
    
    auto lastTick = clock::now();
    auto tickDuration = std::chrono::milliseconds(1000 / m_config.tickRate);
    
    // Counter to track when to check for shutdown
    int checkCounter = 0;
    
    std::cout << "Game thread started" << std::endl;
    
    while (m_running) {
        // Check for shutdown more frequently to ensure timely exit
        if (++checkCounter >= 10) {
            // Re-check the running flag more frequently
            if (!m_running) {
                std::cout << "Game thread detected shutdown flag" << std::endl;
                break;
            }
            checkCounter = 0;
        }
        
        auto currentTime = clock::now();
        auto elapsedTime = currentTime - lastTick;
        
        if (elapsedTime >= tickDuration) {
            // Run a game tick
            tick();
            
            // Calculate how many ticks we should have had
            int64_t ticksToRun = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() / 
                                 (1000 / m_config.tickRate);
            
            // Run additional ticks if we're falling behind, but limit to avoid blocking shutdown
            int64_t maxTicksToRun = std::min(ticksToRun, 
                                           static_cast<int64_t>(m_config.maxUpdatesPerTick));
            
            for (int64_t i = 1; i < maxTicksToRun; ++i) {
                // Check running flag between ticks to allow quick shutdown
                if (!m_running) {
                    break;
                }
                tick();
            }
            
            lastTick = currentTime;
        }
        
        // Use a shorter sleep duration to be more responsive to shutdown requests
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "Game thread exiting" << std::endl;
}