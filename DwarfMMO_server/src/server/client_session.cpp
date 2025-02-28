#include "server/client_session.hpp"
#include "server/server.hpp"
#include <iostream>
#include <thread>

ClientSession::ClientSession(boost::asio::io_context& ioContext, Server* server)
    : m_ioContext(ioContext),
      m_socket(ioContext),
      m_server(server),
      m_connected(false), // Initialize atomic bool
      m_playerId(0),
      m_receiveBuffer(1024),
      m_expectedLength(0),
      m_sending(false) {
}

ClientSession::~ClientSession() {
    close();
}

void ClientSession::start() {
    m_connected.store(true);
    
    // Start receiving data
    startReceive();
    
    // Store this session in the server's client map with a temporary ID
    // Final ID assignment happens in handleConnectRequest
    m_playerId = 0; // temporary
    
    try {
        std::cout << "Client connected: " << m_socket.remote_endpoint() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Client connected (endpoint unavailable)" << std::endl;
    }
}

void ClientSession::sendShutdownNotification() {
    // Only send if still connected
    if (!m_connected.load()) {
        return;
    }
    
    try {
        // Create a disconnect packet with server shutdown message
        DisconnectPacket packet("Server shutting down");
        
        // Use synchronous send to ensure it gets through before socket close
        std::vector<uint8_t> packetData;
        packet.serialize(packetData);
        
        // Add packet length header (4 bytes)
        std::vector<uint8_t> buffer(4 + packetData.size());
        uint32_t length = static_cast<uint32_t>(packetData.size());
        
        buffer[0] = static_cast<uint8_t>((length >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((length >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(length & 0xFF);
        
        // Copy packet data
        std::copy(packetData.begin(), packetData.end(), buffer.begin() + 4);
        
        // Send directly (synchronous)
        boost::system::error_code ec;
        boost::asio::write(m_socket, boost::asio::buffer(buffer), ec);
        
        if (ec) {
            std::cerr << "Error sending shutdown notification: " << ec.message() << std::endl;
        } else {
            std::cout << "Sent shutdown notification to client " << m_playerName << " (ID: " << m_playerId << ")" << std::endl;
        }
        
        // Short delay to allow the packet to be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
    } catch (const std::exception& e) {
        std::cerr << "Error sending shutdown notification: " << e.what() << std::endl;
    }
}

void ClientSession::close() {
    // Use atomic operation to prevent double-close issues
    bool expected = true;
    if (!m_connected.compare_exchange_strong(expected, false)) {
        // Already closed
        return;
    }
    
    // For shutdown tracking
    bool player_removed = false;
    bool socket_closed = false;
    
    // Send shutdown notification first before player cleanup
    sendShutdownNotification();
    
    // Player cleanup - do this first as it's safer
    try {
        // Remove player from the game world
        if (m_player && m_server) {
            m_server->removePlayer(m_playerId);
            player_removed = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error removing player " << m_playerId << ": " << e.what() << std::endl;
    }
    
    // Socket cleanup
    try {
        // Check if socket is open before closing
        if (m_socket.is_open()) {
            // First cancel any pending async operations with timeout
            boost::system::error_code ec;
            
            // First shutdown the socket to ensure clean TCP termination
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec) {
                std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
            }
            
            // Set socket to non-blocking mode to prevent hanging
            m_socket.non_blocking(true, ec);
            if (!ec) {
                // Cancel any pending operations
                m_socket.cancel(ec);
                
                // Now close the socket - this should be non-blocking now
                m_socket.close(ec);
                
                socket_closed = true;
                
                if (ec) {
                    std::cerr << "Error closing socket: " << ec.message() << std::endl;
                }
            } else {
                std::cerr << "Error setting socket to non-blocking mode: " << ec.message() << std::endl;
            }
        } else {
            socket_closed = true;  // Socket was already closed
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during socket closure: " << e.what() << std::endl;
    }
    
    std::cout << "Client disconnected: " << m_playerName 
              << " (ID: " << m_playerId << ")"
              << " [Player removed: " << (player_removed ? "yes" : "no")
              << ", Socket closed: " << (socket_closed ? "yes" : "no") << "]" 
              << std::endl;
}

void ClientSession::sendPacket(const Packet& packet) {
    if (!m_connected.load()) {
        return;
    }
    
    try {
        // Serialize packet
        std::vector<uint8_t> packetData;
        packet.serialize(packetData);
        
        // Add packet length header (4 bytes)
        std::vector<uint8_t> buffer(4 + packetData.size());
        uint32_t length = static_cast<uint32_t>(packetData.size());
        
        buffer[0] = static_cast<uint8_t>((length >> 24) & 0xFF);
        buffer[1] = static_cast<uint8_t>((length >> 16) & 0xFF);
        buffer[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
        buffer[3] = static_cast<uint8_t>(length & 0xFF);
        
        // Copy packet data
        std::copy(packetData.begin(), packetData.end(), buffer.begin() + 4);
        
        // Add to send queue
        m_sendQueue.push(std::move(buffer));
        
        // Start sending if not already sending
        if (!m_sending) {
            startSend();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error preparing packet: " << e.what() << std::endl;
    }
}

void ClientSession::update(float deltaTime) {
    // Update player
    if (m_player) {
        m_player->update(deltaTime, m_server->getWorld());
    }
}

tcp::socket& ClientSession::getSocket() {
    return m_socket;
}

uint32_t ClientSession::getPlayerId() const {
    return m_playerId;
}

std::shared_ptr<Player> ClientSession::getPlayer() const {
    return m_player;
}

void ClientSession::startReceive() {
    if (!m_connected.load()) {
        return;
    }
    
    // Read packet length header (4 bytes)
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_receiveBuffer.data(), 4),
        [self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
            self->handleReceiveHeader(error, bytesTransferred);
        }
    );
}

void ClientSession::handleReceiveHeader(const boost::system::error_code& error, size_t bytesTransferred) {
    if (error) {
        // Only log errors that aren't operation_aborted (server shutdown) or eof (client disconnect)
        if (error != boost::asio::error::operation_aborted && 
            error != boost::asio::error::eof) {
            std::cerr << "Receive header error: " << error.message() << std::endl;
        }
        close();
        return;
    }
    
    // Parse packet length
    m_expectedLength = 
        (static_cast<uint32_t>(m_receiveBuffer[0]) << 24) |
        (static_cast<uint32_t>(m_receiveBuffer[1]) << 16) |
        (static_cast<uint32_t>(m_receiveBuffer[2]) << 8) |
        static_cast<uint32_t>(m_receiveBuffer[3]);
    
    // Sanity check on packet size
    if (m_expectedLength == 0 || m_expectedLength > 1024 * 1024) {
        std::cerr << "Invalid packet size: " << m_expectedLength << std::endl;
        close();
        return;
    }
    
    // Resize buffer if needed
    if (m_receiveBuffer.size() < m_expectedLength) {
        m_receiveBuffer.resize(m_expectedLength);
    }
    
    // Read packet body
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_receiveBuffer.data(), m_expectedLength),
        [self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
            self->handleReceiveBody(error, bytesTransferred);
        }
    );
}

void ClientSession::handleReceiveBody(const boost::system::error_code& error, size_t bytesTransferred) {
    if (error) {
        // Only log errors that aren't operation_aborted (server shutdown) or eof (client disconnect)
        if (error != boost::asio::error::operation_aborted && 
            error != boost::asio::error::eof) {
            std::cerr << "Receive body error: " << error.message() << std::endl;
        }
        close();
        return;
    }
    
    // Process received packet
    processPacket(m_receiveBuffer.data(), bytesTransferred);
    
    // Start receiving next packet
    startReceive();
}

void ClientSession::processPacket(const uint8_t* data, size_t size) {
    try {
        // Create packet from raw data
        auto packet = Packet::createFromRawData(data, size);
        
        if (!packet) {
            std::cerr << "Invalid packet format" << std::endl;
            return;
        }
        
        // Handle packet based on type
        switch (packet->getType()) {
            case PacketType::CONNECT_REQUEST:
                handleConnectRequest(static_cast<const ConnectRequestPacket&>(*packet));
                break;
                
            case PacketType::PLAYER_POSITION:
                handlePlayerPosition(static_cast<const PlayerPositionPacket&>(*packet));
                break;
                
            case PacketType::WORLD_MODIFICATION:
                handleWorldModification(static_cast<const WorldModificationPacket&>(*packet));
                break;
                
            case PacketType::PLAYER_APPEARANCE:
            {
                // Get the appearance info
                const PlayerAppearancePacket& appearancePacket = static_cast<const PlayerAppearancePacket&>(*packet);
                
                // Update the player's appearance
                if (m_player) {
                    SDL_Color color = {
                        appearancePacket.getColorR(),
                        appearancePacket.getColorG(),
                        appearancePacket.getColorB(),
                        255
                    };
                    m_player->setColor(color);
                    m_player->setSymbol(appearancePacket.getSymbol());
                }
                
                // Broadcast to all other clients
                std::lock_guard<std::mutex> lock(m_server->getClientsMutex());
                for (const auto& pair : m_server->getClients()) {
                    // Skip sending to self
                    if (pair.first != m_playerId) {
                        pair.second->sendPacket(appearancePacket);
                    }
                }
                break;
            }
                
            case PacketType::WORLD_CHUNK:
            {
                // Client can't send these, ignore
                break;
            }
                
            case PacketType::PLAYER_LIST:
            {
                // Client can't send these, ignore
                break;
            }
                
            default:
                std::cerr << "Unhandled packet type: " << static_cast<int>(packet->getType()) << std::endl;
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing packet: " << e.what() << std::endl;
    }
}

void ClientSession::startSend() {
    if (!m_connected.load() || m_sendQueue.empty()) {
        m_sending = false;
        return;
    }
    
    m_sending = true;
    
    // Get the next packet from the queue
    const auto& buffer = m_sendQueue.front();
    
    // Send the packet
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(buffer),
        [self = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
            self->handleSend(error, bytesTransferred);
        }
    );
}

void ClientSession::handleSend(const boost::system::error_code& error, size_t bytesTransferred) {
    if (error) {
        // Only log errors that aren't operation_aborted (server shutdown) or eof (client disconnect)
        if (error != boost::asio::error::operation_aborted && 
            error != boost::asio::error::eof) {
            std::cerr << "Send error: " << error.message() << std::endl;
        }
        close();
        return;
    }
    
    // Remove the sent packet from the queue
    m_sendQueue.pop();
    
    // Continue sending if there are more packets
    if (!m_sendQueue.empty()) {
        startSend();
    } else {
        m_sending = false;
    }
}

void ClientSession::sendChunkedWorldState() {
    if (!m_server || !m_player) {
        return;
    }
    
    World* world = m_server->getWorld();
    if (!world) {
        return;
    }
    
    // Player position
    int playerX = m_player->getX();
    int playerY = m_player->getY();
    
    // Send chunks centered around the player
    const int CHUNK_SIZE = 16;
    const int VIEW_DISTANCE = 3; // Number of chunks in each direction
    
    for (int cy = -VIEW_DISTANCE; cy <= VIEW_DISTANCE; ++cy) {
        for (int cx = -VIEW_DISTANCE; cx <= VIEW_DISTANCE; ++cx) {
            // Calculate chunk coordinates
            int chunkX = (playerX / CHUNK_SIZE + cx) * CHUNK_SIZE;
            int chunkY = (playerY / CHUNK_SIZE + cy) * CHUNK_SIZE;
            
            // Create world chunk packet
            WorldChunkPacket chunkPacket(chunkX, chunkY, CHUNK_SIZE, CHUNK_SIZE);
            std::vector<uint8_t> tileData;
            
            // Fill tile data
            for (int y = 0; y < CHUNK_SIZE; ++y) {
                for (int x = 0; x < CHUNK_SIZE; ++x) {
                    int worldX = chunkX + x;
                    int worldY = chunkY + y;
                    
                    Tile tile = world->getTile(worldX, worldY);
                    tileData.push_back(static_cast<uint8_t>(tile.type));
                }
            }
            
            chunkPacket.setTileData(tileData);
            sendPacket(chunkPacket);
        }
    }
}

void ClientSession::handleConnectRequest(const ConnectRequestPacket& packet) {
    // Assign a player ID
    m_playerId = m_server->getNextPlayerId();
    m_playerName = packet.getPlayerName();
    
    std::cout << "Player connected: " << m_playerName << " (ID: " << m_playerId << ")" << std::endl;
    
    // Create a player entity
    m_player = std::make_shared<Player>();
    m_player->setId(m_playerId);
    m_player->setName(m_playerName);
    
    // Place player in the world - THIS ADDS THE PLAYER AND BROADCASTS POSITION
    m_server->addPlayer(m_playerId, m_player);
    
    // Send connection accepted packet
    ConnectAcceptPacket acceptPacket(m_playerId);
    sendPacket(acceptPacket);
    
    // FIRST, send player appearance packet to the new player for themselves
    SDL_Color color = m_player->getColor();
    PlayerAppearancePacket selfAppearancePacket(m_playerId, m_player->getSymbol(), 
                                           color.r, color.g, color.b, 
                                           m_playerName);
    sendPacket(selfAppearancePacket);
    
    // THEN send the appearance to all other players
    {
        std::lock_guard<std::mutex> lock(m_server->getClientsMutex());
        for (const auto& pair : m_server->getClients()) {
            if (pair.first != m_playerId) {
                PlayerAppearancePacket newPlayerPacket(m_playerId, m_player->getSymbol(),
                                                    color.r, color.g, color.b,
                                                    m_playerName);
                pair.second->sendPacket(newPlayerPacket);
                
                // Also send each existing player's appearance to the new player
                auto otherPlayer = pair.second->getPlayer();
                if (otherPlayer) {
                    SDL_Color otherColor = otherPlayer->getColor();
                    PlayerAppearancePacket otherAppearancePacket(
                        otherPlayer->getId(), 
                        otherPlayer->getSymbol(),
                        otherColor.r, otherColor.g, otherColor.b,
                        otherPlayer->getName()
                    );
                    sendPacket(otherAppearancePacket);
                }
            }
        }
    }
    
    // IMPORTANT: First register this client in the server's client map
    // Otherwise other clients won't see it in the client map when they try to send player list
    {
        std::lock_guard<std::mutex> lock(m_server->getClientsMutex());
        m_server->getClients()[m_playerId] = shared_from_this();
    }
    
    // Send existing players position info to the new player
    PlayerListPacket playerListPacket;
    std::lock_guard<std::mutex> lock(m_server->getClientsMutex());
    for (const auto& pair : m_server->getClients()) {
        if (pair.first != m_playerId) {
            auto otherPlayer = pair.second->getPlayer();
            if (otherPlayer) {
                playerListPacket.addPlayer(otherPlayer->getId(), 
                                          otherPlayer->getName(),
                                          otherPlayer->getX(), 
                                          otherPlayer->getY());
            }
        }
    }
    
    // Send the player list to the new player
    if (!playerListPacket.getPlayers().empty()) {
        sendPacket(playerListPacket);
    }
    
    // Send initial world state
    sendChunkedWorldState();
}

void ClientSession::handlePlayerPosition(const PlayerPositionPacket& packet) {
    if (!m_player) {
        return;
    }
    
    // Verify player ID
    if (packet.getPlayerId() != m_playerId) {
        std::cerr << "Player ID mismatch: " << packet.getPlayerId() << " != " << m_playerId << std::endl;
        return;
    }
    
    // Update player position
    m_player->setPosition(packet.getX(), packet.getY());
    
    // Broadcast to other clients
    m_server->broadcastPlayerPosition(m_playerId, packet.getX(), packet.getY());
}

void ClientSession::handleWorldModification(const WorldModificationPacket& packet) {
    if (!m_player) {
        return;
    }
    
    // Get tile type
    TileType tileType = static_cast<TileType>(packet.getTileType());
    
    // Get player position
    int playerX = m_player->getX();
    int playerY = m_player->getY();
    
    // Check if the modification is within range
    int dx = packet.getX() - playerX;
    int dy = packet.getY() - playerY;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance <= m_server->getConfig().playerInteractRange) {
        // Modify the world
        m_server->getWorld()->setTile(packet.getX(), packet.getY(), tileType);
        
        // Broadcast to ALL clients including the sender
        m_server->broadcastWorldModification(packet.getX(), packet.getY(), packet.getTileType());
    }
}