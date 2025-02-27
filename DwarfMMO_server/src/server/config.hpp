#pragma once

#include <string>
#include <cstdint>

struct ServerConfig {
    // Network settings
    uint16_t port = 7777;
    uint32_t maxClients = 100;
    uint32_t tickRate = 20;  // Updates per second
    
    // World settings
    int worldWidth = 500;
    int worldHeight = 500;
    int worldDepth = 10;     // Z layers
    std::string worldSeed = "dwarf_mmo";
    
    // Player settings
    float playerMoveSpeed = 5.0f;  // Tiles per second
    float playerInteractRange = 5.0f;
    
    // Performance settings
    uint32_t maxUpdatesPerTick = 1000;
    uint32_t chunkSize = 16;
    
    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    
    // Save configuration to file
    bool saveToFile(const std::string& filename) const;
};