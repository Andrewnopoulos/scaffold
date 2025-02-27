#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

// Packet types enum
enum class PacketType : uint8_t {
    CONNECT_REQUEST = 1,
    CONNECT_ACCEPT,
    DISCONNECT,
    PING,
    PONG,
    PLAYER_POSITION,
    PLAYER_APPEARANCE,
    WORLD_CHUNK,
    WORLD_MODIFICATION,
    CHAT_MESSAGE,
    PLAYER_LIST
};

class Packet {
public:
    virtual ~Packet() = default;
    
    // Serialize packet data to buffer
    virtual void serialize(std::vector<uint8_t>& buffer) const = 0;
    
    // Deserialize packet data from buffer
    virtual bool deserialize(const uint8_t* data, size_t size) = 0;
    
    // Get packet type
    virtual PacketType getType() const = 0;
    
    // Create packet from raw data
    static std::unique_ptr<Packet> createFromRawData(const uint8_t* data, size_t size);
};

// Connection request packet
class ConnectRequestPacket : public Packet {
public:
    ConnectRequestPacket(const std::string& playerName = "");
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::CONNECT_REQUEST; }
    
    const std::string& getPlayerName() const { return m_playerName; }
    
private:
    std::string m_playerName;
};

// Connection accept packet
class ConnectAcceptPacket : public Packet {
public:
    ConnectAcceptPacket(uint32_t playerId = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::CONNECT_ACCEPT; }
    
    uint32_t getPlayerId() const { return m_playerId; }
    
private:
    uint32_t m_playerId;
};

// Disconnect packet
class DisconnectPacket : public Packet {
public:
    DisconnectPacket(const std::string& reason = "");
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::DISCONNECT; }
    
    const std::string& getReason() const { return m_reason; }
    
private:
    std::string m_reason;
};

// Ping packet
class PingPacket : public Packet {
public:
    PingPacket(uint32_t timestamp = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::PING; }
    
    uint32_t getTimestamp() const { return m_timestamp; }
    
private:
    uint32_t m_timestamp;
};

// Pong packet
class PongPacket : public Packet {
public:
    PongPacket(uint32_t timestamp = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::PONG; }
    
    uint32_t getTimestamp() const { return m_timestamp; }
    
private:
    uint32_t m_timestamp;
};

// Player position packet
class PlayerPositionPacket : public Packet {
public:
    PlayerPositionPacket(uint32_t playerId = 0, int32_t x = 0, int32_t y = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::PLAYER_POSITION; }
    
    uint32_t getPlayerId() const { return m_playerId; }
    int32_t getX() const { return m_x; }
    int32_t getY() const { return m_y; }
    
private:
    uint32_t m_playerId;
    int32_t m_x;
    int32_t m_y;
};

// Player appearance packet
class PlayerAppearancePacket : public Packet {
public:
    PlayerAppearancePacket(uint32_t playerId = 0, char symbol = '@', 
                          uint8_t colorR = 255, uint8_t colorG = 255, uint8_t colorB = 255,
                          const std::string& name = "");
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::PLAYER_APPEARANCE; }
    
    uint32_t getPlayerId() const { return m_playerId; }
    char getSymbol() const { return m_symbol; }
    uint8_t getColorR() const { return m_colorR; }
    uint8_t getColorG() const { return m_colorG; }
    uint8_t getColorB() const { return m_colorB; }
    const std::string& getName() const { return m_name; }
    
private:
    uint32_t m_playerId;
    char m_symbol;
    uint8_t m_colorR;
    uint8_t m_colorG;
    uint8_t m_colorB;
    std::string m_name;
};

// World modification packet
class WorldModificationPacket : public Packet {
public:
    WorldModificationPacket(int32_t x = 0, int32_t y = 0, uint8_t tileType = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::WORLD_MODIFICATION; }
    
    int32_t getX() const { return m_x; }
    int32_t getY() const { return m_y; }
    uint8_t getTileType() const { return m_tileType; }
    
private:
    int32_t m_x;
    int32_t m_y;
    uint8_t m_tileType;
};

// World chunk packet
class WorldChunkPacket : public Packet {
public:
    WorldChunkPacket(int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::WORLD_CHUNK; }
    
    int32_t getX() const { return m_x; }
    int32_t getY() const { return m_y; }
    int32_t getWidth() const { return m_width; }
    int32_t getHeight() const { return m_height; }
    
    const std::vector<uint8_t>& getTileData() const { return m_tileData; }
    void setTileData(const std::vector<uint8_t>& tileData) { m_tileData = tileData; }
    
private:
    int32_t m_x;
    int32_t m_y;
    int32_t m_width;
    int32_t m_height;
    std::vector<uint8_t> m_tileData;
};

// Chat message packet
class ChatMessagePacket : public Packet {
public:
    ChatMessagePacket(uint32_t playerId = 0, const std::string& message = "");
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::CHAT_MESSAGE; }
    
    uint32_t getPlayerId() const { return m_playerId; }
    const std::string& getMessage() const { return m_message; }
    
private:
    uint32_t m_playerId;
    std::string m_message;
};

// Player list packet
class PlayerListPacket : public Packet {
public:
    struct PlayerInfo {
        uint32_t id;
        std::string name;
        int32_t x;
        int32_t y;
    };
    
    PlayerListPacket() = default;
    
    void serialize(std::vector<uint8_t>& buffer) const override;
    bool deserialize(const uint8_t* data, size_t size) override;
    PacketType getType() const override { return PacketType::PLAYER_LIST; }
    
    const std::vector<PlayerInfo>& getPlayers() const { return m_players; }
    void addPlayer(uint32_t id, const std::string& name, int32_t x, int32_t y);
    
private:
    std::vector<PlayerInfo> m_players;
};

// Utility functions for serialization
void writeUint8(std::vector<uint8_t>& buffer, uint8_t value);
void writeUint16(std::vector<uint8_t>& buffer, uint16_t value);
void writeUint32(std::vector<uint8_t>& buffer, uint32_t value);
void writeInt32(std::vector<uint8_t>& buffer, int32_t value);
void writeString(std::vector<uint8_t>& buffer, const std::string& value);

uint8_t readUint8(const uint8_t* data, size_t& offset, size_t size);
uint16_t readUint16(const uint8_t* data, size_t& offset, size_t size);
uint32_t readUint32(const uint8_t* data, size_t& offset, size_t size);
int32_t readInt32(const uint8_t* data, size_t& offset, size_t size);
std::string readString(const uint8_t* data, size_t& offset, size_t size);