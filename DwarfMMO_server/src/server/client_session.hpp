#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <queue>
#include <vector>
#include <atomic>
#include "network/packet.hpp"
#include "game/player.hpp"

using boost::asio::ip::tcp;

// Forward declarations
class Server;
class World;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::io_context& ioContext, Server* server);
    ~ClientSession();
    
    // Start the session
    void start();
    
    // Send a shutdown notification to client
    void sendShutdownNotification();
    
    // Close the session
    void close();
    
    // Send a packet to the client
    void sendPacket(const Packet& packet);
    
    // Process a single game update tick
    void update(float deltaTime);
    
    // Get the TCP socket
    tcp::socket& getSocket();
    
    // Get the player ID
    uint32_t getPlayerId() const;
    
    // Get the player entity
    std::shared_ptr<Player> getPlayer() const;
    
private:
    // Network state
    boost::asio::io_context& m_ioContext;
    tcp::socket m_socket;
    Server* m_server;
    std::atomic<bool> m_connected;
    
    // Player data
    uint32_t m_playerId;
    std::string m_playerName;
    std::shared_ptr<Player> m_player;
    
    // Receive buffer
    std::vector<uint8_t> m_receiveBuffer;
    uint32_t m_expectedLength;
    
    // Send queue
    std::queue<std::vector<uint8_t>> m_sendQueue;
    bool m_sending;
    
    // Start receiving data
    void startReceive();

    void sendChunkedWorldState();
    
    // Handle received header
    void handleReceiveHeader(const boost::system::error_code& error, size_t bytesTransferred);
    
    // Handle received body
    void handleReceiveBody(const boost::system::error_code& error, size_t bytesTransferred);
    
    // Process received packet
    void processPacket(const uint8_t* data, size_t size);
    
    // Start sending queued packets
    void startSend();
    
    // Handle send completion
    void handleSend(const boost::system::error_code& error, size_t bytesTransferred);
    
    // Packet handlers
    void handleConnectRequest(const ConnectRequestPacket& packet);
    void handlePlayerPosition(const PlayerPositionPacket& packet);
    void handleWorldModification(const WorldModificationPacket& packet);
};

using ClientSessionPtr = std::shared_ptr<ClientSession>;