#include "network/client.hpp"
#include <iostream>

NetworkClient::NetworkClient()
    : m_socket(m_ioContext),
      m_connected(false),
      m_receiveBuffer(1024),
      m_expectedLength(0) {
    
    // Start io_context in a separate thread
    m_work = std::make_unique<boost::asio::io_context::work>(m_ioContext);
    m_networkThread = std::thread([this]() {
        try {
            m_ioContext.run();
        } catch (const std::exception& e) {
            std::cerr << "Network thread exception: " << e.what() << std::endl;
        }
    });
}

NetworkClient::~NetworkClient() {
    disconnect();
    
    // Stop io_context and join thread
    m_work.reset();
    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }
}

bool NetworkClient::connect(const std::string& host, uint16_t port) {
    try {
        if (m_connected) {
            disconnect();
        }
        
        // Resolve endpoint
        tcp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        
        // Connect to server
        boost::asio::connect(m_socket, endpoints);
        
        // Start receiving data
        m_connected = true;
        startReceive();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return false;
    }
}

void NetworkClient::disconnect() {
    if (!m_connected) {
        return;
    }
    
    try {
        m_socket.close();
    } catch (const std::exception& e) {
        std::cerr << "Disconnect error: " << e.what() << std::endl;
    }
    
    m_connected = false;
}

bool NetworkClient::isConnected() const {
    return m_connected && m_socket.is_open();
}

void NetworkClient::sendPacket(const Packet& packet) {
    if (!isConnected()) {
        return;
    }
    
    try {
        // Serialize packet
        std::vector<uint8_t> buffer;
        packet.serialize(buffer);
        
        // Add packet length header (4 bytes)
        std::vector<uint8_t> header(4);
        uint32_t length = static_cast<uint32_t>(buffer.size());
        header[0] = static_cast<uint8_t>((length >> 24) & 0xFF);
        header[1] = static_cast<uint8_t>((length >> 16) & 0xFF);
        header[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
        header[3] = static_cast<uint8_t>(length & 0xFF);
        
        // Create combined buffer for sending
        std::vector<boost::asio::const_buffer> sendBuffers = {
            boost::asio::buffer(header),
            boost::asio::buffer(buffer)
        };
        
        // Send packet
        boost::asio::write(m_socket, sendBuffers);
    } catch (const std::exception& e) {
        std::cerr << "Send error: " << e.what() << std::endl;
        disconnect();
    }
}

void NetworkClient::update() {
    std::lock_guard<std::mutex> lock(m_packetQueueMutex);
    
    if (!m_packetQueue.empty()) {
        std::cout << "Processing " << m_packetQueue.size() << " queued packets" << std::endl;
    }
    
    while (!m_packetQueue.empty()) {
        auto& packet = m_packetQueue.front();
        
        // Log each packet we're processing
        std::cout << "Processing packet of type: " << static_cast<int>(packet->getType()) << std::endl;
        
        // Call packet handler if registered
        auto handlerIt = m_packetHandlers.find(static_cast<uint8_t>(packet->getType()));
        if (handlerIt != m_packetHandlers.end()) {
            std::cout << "Handler found for packet type: " << static_cast<int>(packet->getType()) << std::endl;
            handlerIt->second(*packet);
        } else {
            std::cout << "No handler registered for packet type: " << static_cast<int>(packet->getType()) << std::endl;
        }
        
        m_packetQueue.pop();
    }
}

void NetworkClient::startReceive() {
    if (!isConnected()) {
        return;
    }
    
    // Read packet length header (4 bytes)
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_receiveBuffer.data(), 4),
        [this](const boost::system::error_code& error, size_t bytesTransferred) {
            handleReceiveHeader(error, bytesTransferred);
        }
    );
}

void NetworkClient::handleReceiveHeader(const boost::system::error_code& error, size_t bytesTransferred) {
    if (error) {
        // Only log if it's not just a clean disconnect
        if (error != boost::asio::error::operation_aborted && 
            error != boost::asio::error::connection_reset &&
            error != boost::asio::error::eof) {
            std::cerr << "Receive header error: " << error.message() << std::endl;
        } else {
            std::cout << "Server disconnected: " << error.message() << std::endl;
        }
        
        // Notify about disconnection
        bool wasConnected = m_connected;
        disconnect();
        
        if (wasConnected) {
            std::cout << "Connection to server lost." << std::endl;
            
            // Queue a "Server disconnected" notification packet
            try {
                auto disconnectPacket = std::make_unique<DisconnectPacket>("Server disconnected unexpectedly");
                std::lock_guard<std::mutex> lock(m_packetQueueMutex);
                m_packetQueue.push(std::move(disconnectPacket));
            } catch (const std::exception& e) {
                std::cerr << "Error creating disconnect notification: " << e.what() << std::endl;
            }
        }
        
        return;
    }
    
    // Parse packet length
    m_expectedLength = 
        (static_cast<uint32_t>(m_receiveBuffer[0]) << 24) |
        (static_cast<uint32_t>(m_receiveBuffer[1]) << 16) |
        (static_cast<uint32_t>(m_receiveBuffer[2]) << 8) |
         static_cast<uint32_t>(m_receiveBuffer[3]);
    
    // Resize buffer if needed
    if (m_receiveBuffer.size() < m_expectedLength) {
        m_receiveBuffer.resize(m_expectedLength);
    }
    
    // Read packet body
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_receiveBuffer.data(), m_expectedLength),
        [this](const boost::system::error_code& error, size_t bytesTransferred) {
            handleReceiveBody(error, bytesTransferred);
        }
    );
}

void NetworkClient::handleReceiveBody(const boost::system::error_code& error, size_t bytesTransferred) {
    if (error) {
        // Only log if it's not just a clean disconnect
        if (error != boost::asio::error::operation_aborted && 
            error != boost::asio::error::connection_reset &&
            error != boost::asio::error::eof) {
            std::cerr << "Receive body error: " << error.message() << std::endl;
        } else {
            std::cout << "Server disconnected: " << error.message() << std::endl;
        }
        
        // Notify about disconnection
        bool wasConnected = m_connected;
        disconnect();
        
        if (wasConnected) {
            std::cout << "Connection to server lost during packet receive." << std::endl;
            
            // Queue a "Server disconnected" notification packet
            try {
                auto disconnectPacket = std::make_unique<DisconnectPacket>("Server disconnected unexpectedly");
                std::lock_guard<std::mutex> lock(m_packetQueueMutex);
                m_packetQueue.push(std::move(disconnectPacket));
            } catch (const std::exception& e) {
                std::cerr << "Error creating disconnect notification: " << e.what() << std::endl;
            }
        }
        
        return;
    }
    
    // Process received packet
    processPacket(m_receiveBuffer.data(), bytesTransferred);
    
    // Start receiving next packet
    startReceive();
}

void NetworkClient::processPacket(const uint8_t* data, size_t size) {
    std::cout << "NetworkClient::processPacket - Processing " << size << " bytes" << std::endl;
    
    // Create packet from raw data
    auto packet = Packet::createFromRawData(data, size);
    
    if (packet) {
        std::cout << "Created packet of type: " << static_cast<int>(packet->getType()) << std::endl;
        
        // Add packet to queue
        std::lock_guard<std::mutex> lock(m_packetQueueMutex);
        m_packetQueue.push(std::move(packet));
    } else {
        std::cout << "Failed to create packet from received data" << std::endl;
    }
}