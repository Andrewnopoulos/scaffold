#pragma once

#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include "server/config.hpp"
#include "game/world.hpp"

using boost::asio::ip::tcp;

// Forward declarations
class ClientSession;
using ClientSessionPtr = std::shared_ptr<ClientSession>;

class Server : public std::enable_shared_from_this<Server> {
public:
    Server(boost::asio::io_context& ioContext, const ServerConfig& config);
    ~Server();
    
    // Start the server
    void start();
    
    // Stop the server
    void stop();
    
    // Run a single game update tick
    void tick();
    
    // Add a player to the world
    void addPlayer(uint32_t playerId, std::shared_ptr<Player> player);
    
    // Remove a player from the world
    void removePlayer(uint32_t playerId);
    
    // Get the next available player ID
    uint32_t getNextPlayerId();
    
    // Get the world
    World* getWorld();
    
    // Get the server configuration
    const ServerConfig& getConfig() const;
    
    // Broadcast player position to all clients
    void broadcastPlayerPosition(uint32_t playerId, int x, int y);
    
    // Broadcast world modification to all clients
    void broadcastWorldModification(int x, int y, uint8_t tileType);

    // Get the clients mutex (for synchronized access)
    std::mutex& getClientsMutex() { return m_clientsMutex; }
    
    // Get all connected clients
    const std::unordered_map<uint32_t, ClientSessionPtr>& getClients() const { return m_clients; }
    std::unordered_map<uint32_t, ClientSessionPtr>& getClients() { return m_clients; }
    
private:
    // Server state
    boost::asio::io_context& m_ioContext;
    ServerConfig m_config;
    std::atomic<bool> m_running;
    
    // Networking
    tcp::acceptor m_acceptor;
    std::mutex m_clientsMutex;
    std::unordered_map<uint32_t, ClientSessionPtr> m_clients;
    
    // Game state
    std::unique_ptr<World> m_world;
    std::atomic<uint32_t> m_nextPlayerId;
    
    // Game loop thread
    std::thread m_gameThread;
    
    // Start accepting connections
    void startAccept();
    
    // Handle new connection
    void handleAccept(ClientSessionPtr session, const boost::system::error_code& error);
    
    // Game loop function
    void gameLoop();
};

using ServerPtr = std::shared_ptr<Server>;