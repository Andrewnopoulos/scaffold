#pragma once

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <boost/asio.hpp>
#include <functional>
#include <mutex>
#include "network/packet.hpp"

using boost::asio::ip::tcp;

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    
    // Connect to server
    bool connect(const std::string& host, uint16_t port);
    
    // Disconnect from server
    void disconnect();
    
    // Check if connected
    bool isConnected() const;
    
    // Send packet to server
    void sendPacket(const Packet& packet);
    
    // Process received packets
    void update();
    
    // Set packet handler callback
    template<typename T>
    void setPacketHandler(std::function<void(const T&)> handler) {
        static_assert(std::is_base_of<Packet, T>::value, "T must derive from Packet");
        m_packetHandlers[static_cast<uint8_t>(T().getType())] = [handler](const Packet& packet) {
            handler(static_cast<const T&>(packet));
        };
    }
    
private:
    // Network state
    boost::asio::io_context m_ioContext;
    tcp::socket m_socket;
    std::unique_ptr<boost::asio::io_context::work> m_work;
    std::thread m_networkThread;
    bool m_connected;
    
    // Receive buffer
    std::vector<uint8_t> m_receiveBuffer;
    uint32_t m_expectedLength;
    
    // Packet handling
    std::mutex m_packetQueueMutex;
    std::queue<std::unique_ptr<Packet>> m_packetQueue;
    std::unordered_map<uint8_t, std::function<void(const Packet&)>> m_packetHandlers;
    
    // Start receiving data
    void startReceive();
    
    // Handle received header
    void handleReceiveHeader(const boost::system::error_code& error, size_t bytesTransferred);
    
    // Handle received data
    void handleReceiveBody(const boost::system::error_code& error, size_t bytesTransferred);
    
    // Process a single received packet
    void processPacket(const uint8_t* data, size_t size);
};