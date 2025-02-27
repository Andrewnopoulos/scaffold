#include "network/packet.hpp"
#include <cstring>
#include <stdexcept>

// Utility functions for serialization
void writeUint8(std::vector<uint8_t>& buffer, uint8_t value) {
    buffer.push_back(value);
}

void writeUint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void writeUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void writeInt32(std::vector<uint8_t>& buffer, int32_t value) {
    writeUint32(buffer, static_cast<uint32_t>(value));
}

void writeString(std::vector<uint8_t>& buffer, const std::string& value) {
    // Write string length
    writeUint16(buffer, static_cast<uint16_t>(value.length()));
    
    // Write string data
    for (char c : value) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

uint8_t readUint8(const uint8_t* data, size_t& offset, size_t size) {
    if (offset + 1 > size) {
        throw std::runtime_error("Buffer overflow when reading uint8");
    }
    
    uint8_t value = data[offset];
    offset += 1;
    return value;
}

uint16_t readUint16(const uint8_t* data, size_t& offset, size_t size) {
    if (offset + 2 > size) {
        throw std::runtime_error("Buffer overflow when reading uint16");
    }
    
    uint16_t value = (static_cast<uint16_t>(data[offset]) << 8) |
                      static_cast<uint16_t>(data[offset + 1]);
    offset += 2;
    return value;
}

uint32_t readUint32(const uint8_t* data, size_t& offset, size_t size) {
    if (offset + 4 > size) {
        throw std::runtime_error("Buffer overflow when reading uint32");
    }
    
    uint32_t value = (static_cast<uint32_t>(data[offset]) << 24) |
                     (static_cast<uint32_t>(data[offset + 1]) << 16) |
                     (static_cast<uint32_t>(data[offset + 2]) << 8) |
                      static_cast<uint32_t>(data[offset + 3]);
    offset += 4;
    return value;
}

int32_t readInt32(const uint8_t* data, size_t& offset, size_t size) {
    return static_cast<int32_t>(readUint32(data, offset, size));
}

std::string readString(const uint8_t* data, size_t& offset, size_t size) {
    // Read string length
    uint16_t length = readUint16(data, offset, size);
    
    if (offset + length > size) {
        throw std::runtime_error("Buffer overflow when reading string");
    }
    
    // Read string data
    std::string value(reinterpret_cast<const char*>(data + offset), length);
    offset += length;
    
    return value;
}

// Packet factory method
std::unique_ptr<Packet> Packet::createFromRawData(const uint8_t* data, size_t size) {
    if (size < 1) {
        return nullptr;
    }
    
    PacketType type = static_cast<PacketType>(data[0]);
    size_t offset = 1; // Skip packet type
    
    std::unique_ptr<Packet> packet;
    
    switch (type) {
        case PacketType::CONNECT_REQUEST:
            packet = std::make_unique<ConnectRequestPacket>();
            break;
        case PacketType::CONNECT_ACCEPT:
            packet = std::make_unique<ConnectAcceptPacket>();
            break;
        case PacketType::DISCONNECT:
            packet = std::make_unique<DisconnectPacket>();
            break;
        case PacketType::PING:
            packet = std::make_unique<PingPacket>();
            break;
        case PacketType::PONG:
            packet = std::make_unique<PongPacket>();
            break;
        case PacketType::PLAYER_POSITION:
            packet = std::make_unique<PlayerPositionPacket>();
            break;
        case PacketType::PLAYER_APPEARANCE:
            packet = std::make_unique<PlayerAppearancePacket>();
            break;
        case PacketType::WORLD_CHUNK:
            packet = std::make_unique<WorldChunkPacket>();
            break;
        case PacketType::WORLD_MODIFICATION:
            packet = std::make_unique<WorldModificationPacket>();
            break;
        case PacketType::CHAT_MESSAGE:
            packet = std::make_unique<ChatMessagePacket>();
            break;
        case PacketType::PLAYER_LIST:
            packet = std::make_unique<PlayerListPacket>();
            break;
        default:
            return nullptr;
    }
    
    if (!packet->deserialize(data + 1, size - 1)) {
        return nullptr;
    }
    
    return packet;
}

// ConnectRequestPacket implementation
ConnectRequestPacket::ConnectRequestPacket(const std::string& playerName)
    : m_playerName(playerName) {
}

void ConnectRequestPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player name
    writeString(buffer, m_playerName);
}

bool ConnectRequestPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_playerName = readString(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ConnectAcceptPacket implementation
ConnectAcceptPacket::ConnectAcceptPacket(uint32_t playerId)
    : m_playerId(playerId) {
}

void ConnectAcceptPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player ID
    writeUint32(buffer, m_playerId);
}

bool ConnectAcceptPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_playerId = readUint32(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// DisconnectPacket implementation
DisconnectPacket::DisconnectPacket(const std::string& reason)
    : m_reason(reason) {
}

void DisconnectPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write reason
    writeString(buffer, m_reason);
}

bool DisconnectPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_reason = readString(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// PingPacket implementation
PingPacket::PingPacket(uint32_t timestamp)
    : m_timestamp(timestamp) {
}

void PingPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write timestamp
    writeUint32(buffer, m_timestamp);
}

bool PingPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_timestamp = readUint32(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// PongPacket implementation
PongPacket::PongPacket(uint32_t timestamp)
    : m_timestamp(timestamp) {
}

void PongPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write timestamp
    writeUint32(buffer, m_timestamp);
}

bool PongPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_timestamp = readUint32(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// PlayerPositionPacket implementation
PlayerPositionPacket::PlayerPositionPacket(uint32_t playerId, int32_t x, int32_t y)
    : m_playerId(playerId), m_x(x), m_y(y) {
}

void PlayerPositionPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player ID
    writeUint32(buffer, m_playerId);
    
    // Write position
    writeInt32(buffer, m_x);
    writeInt32(buffer, m_y);
}

bool PlayerPositionPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_playerId = readUint32(data, offset, size);
        m_x = readInt32(data, offset, size);
        m_y = readInt32(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// PlayerAppearancePacket implementation
PlayerAppearancePacket::PlayerAppearancePacket(uint32_t playerId, char symbol, 
                                     uint8_t colorR, uint8_t colorG, uint8_t colorB,
                                     const std::string& name)
    : m_playerId(playerId), m_symbol(symbol), 
      m_colorR(colorR), m_colorG(colorG), m_colorB(colorB),
      m_name(name) {
}

void PlayerAppearancePacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player ID
    writeUint32(buffer, m_playerId);
    
    // Write symbol
    writeUint8(buffer, static_cast<uint8_t>(m_symbol));
    
    // Write color
    writeUint8(buffer, m_colorR);
    writeUint8(buffer, m_colorG);
    writeUint8(buffer, m_colorB);
    
    // Write name
    writeString(buffer, m_name);
}

bool PlayerAppearancePacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_playerId = readUint32(data, offset, size);
        m_symbol = static_cast<char>(readUint8(data, offset, size));
        m_colorR = readUint8(data, offset, size);
        m_colorG = readUint8(data, offset, size);
        m_colorB = readUint8(data, offset, size);
        m_name = readString(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// WorldModificationPacket implementation
WorldModificationPacket::WorldModificationPacket(int32_t x, int32_t y, uint8_t tileType)
    : m_x(x), m_y(y), m_tileType(tileType) {
}

void WorldModificationPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write position
    writeInt32(buffer, m_x);
    writeInt32(buffer, m_y);
    
    // Write tile type
    writeUint8(buffer, m_tileType);
}

bool WorldModificationPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_x = readInt32(data, offset, size);
        m_y = readInt32(data, offset, size);
        m_tileType = readUint8(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// WorldChunkPacket implementation
WorldChunkPacket::WorldChunkPacket(int32_t x, int32_t y, int32_t width, int32_t height)
    : m_x(x), m_y(y), m_width(width), m_height(height) {
}

void WorldChunkPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write chunk position and size
    writeInt32(buffer, m_x);
    writeInt32(buffer, m_y);
    writeInt32(buffer, m_width);
    writeInt32(buffer, m_height);
    
    // Write tile data size
    writeUint32(buffer, static_cast<uint32_t>(m_tileData.size()));
    
    // Write tile data
    buffer.insert(buffer.end(), m_tileData.begin(), m_tileData.end());
}

bool WorldChunkPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_x = readInt32(data, offset, size);
        m_y = readInt32(data, offset, size);
        m_width = readInt32(data, offset, size);
        m_height = readInt32(data, offset, size);
        
        uint32_t dataSize = readUint32(data, offset, size);
        
        if (offset + dataSize > size) {
            throw std::runtime_error("Buffer overflow when reading tile data");
        }
        
        m_tileData.assign(data + offset, data + offset + dataSize);
        offset += dataSize;
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ChatMessagePacket implementation
ChatMessagePacket::ChatMessagePacket(uint32_t playerId, const std::string& message)
    : m_playerId(playerId), m_message(message) {
}

void ChatMessagePacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player ID
    writeUint32(buffer, m_playerId);
    
    // Write message
    writeString(buffer, m_message);
}

bool ChatMessagePacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        m_playerId = readUint32(data, offset, size);
        m_message = readString(data, offset, size);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// PlayerListPacket implementation
void PlayerListPacket::addPlayer(uint32_t id, const std::string& name, int32_t x, int32_t y) {
    PlayerInfo info;
    info.id = id;
    info.name = name;
    info.x = x;
    info.y = y;
    m_players.push_back(info);
}

void PlayerListPacket::serialize(std::vector<uint8_t>& buffer) const {
    // Write packet type
    writeUint8(buffer, static_cast<uint8_t>(getType()));
    
    // Write player count
    writeUint16(buffer, static_cast<uint16_t>(m_players.size()));
    
    // Write player info
    for (const auto& player : m_players) {
        writeUint32(buffer, player.id);
        writeString(buffer, player.name);
        writeInt32(buffer, player.x);
        writeInt32(buffer, player.y);
    }
}

bool PlayerListPacket::deserialize(const uint8_t* data, size_t size) {
    try {
        size_t offset = 0;
        uint16_t playerCount = readUint16(data, offset, size);
        
        m_players.clear();
        m_players.reserve(playerCount);
        
        for (uint16_t i = 0; i < playerCount; ++i) {
            PlayerInfo info;
            info.id = readUint32(data, offset, size);
            info.name = readString(data, offset, size);
            info.x = readInt32(data, offset, size);
            info.y = readInt32(data, offset, size);
            
            m_players.push_back(info);
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}