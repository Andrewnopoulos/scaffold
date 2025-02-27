#include "server/config.hpp"
#include <fstream>
#include <iostream>

// In a real implementation, you'd want to use a proper JSON/YAML/XML parser
// For simplicity, we'll use a basic implementation here

bool ServerConfig::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse key=value pairs
            size_t delimPos = line.find('=');
            if (delimPos != std::string::npos) {
                std::string key = line.substr(0, delimPos);
                std::string value = line.substr(delimPos + 1);
                
                // Remove whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Set configuration values
                if (key == "port") {
                    port = static_cast<uint16_t>(std::stoi(value));
                } else if (key == "maxClients") {
                    maxClients = static_cast<uint32_t>(std::stoi(value));
                } else if (key == "tickRate") {
                    tickRate = static_cast<uint32_t>(std::stoi(value));
                } else if (key == "worldWidth") {
                    worldWidth = std::stoi(value);
                } else if (key == "worldHeight") {
                    worldHeight = std::stoi(value);
                } else if (key == "worldDepth") {
                    worldDepth = std::stoi(value);
                } else if (key == "worldSeed") {
                    worldSeed = value;
                } else if (key == "playerMoveSpeed") {
                    playerMoveSpeed = std::stof(value);
                } else if (key == "playerInteractRange") {
                    playerInteractRange = std::stof(value);
                } else if (key == "maxUpdatesPerTick") {
                    maxUpdatesPerTick = static_cast<uint32_t>(std::stoi(value));
                } else if (key == "chunkSize") {
                    chunkSize = static_cast<uint32_t>(std::stoi(value));
                }
            }
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool ServerConfig::saveToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filename << std::endl;
            return false;
        }
        
        file << "# DwarfMMO Server Configuration\n\n";
        
        // Network settings
        file << "# Network settings\n";
        file << "port=" << port << "\n";
        file << "maxClients=" << maxClients << "\n";
        file << "tickRate=" << tickRate << "\n\n";
        
        // World settings
        file << "# World settings\n";
        file << "worldWidth=" << worldWidth << "\n";
        file << "worldHeight=" << worldHeight << "\n";
        file << "worldDepth=" << worldDepth << "\n";
        file << "worldSeed=" << worldSeed << "\n\n";
        
        // Player settings
        file << "# Player settings\n";
        file << "playerMoveSpeed=" << playerMoveSpeed << "\n";
        file << "playerInteractRange=" << playerInteractRange << "\n\n";
        
        // Performance settings
        file << "# Performance settings\n";
        file << "maxUpdatesPerTick=" << maxUpdatesPerTick << "\n";
        file << "chunkSize=" << chunkSize << "\n";
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}