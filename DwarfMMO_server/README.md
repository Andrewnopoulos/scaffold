# DwarfMMO Server

This is the server component for the DwarfMMO game. It handles client connections, world state, and player synchronization.

## Features

- TCP-based networking using Boost.Asio
- Multi-threaded design with separate networking and game update threads
- World state management with tile-based terrain
- Player entity management
- Packet-based communication protocol

## Building the Server

### Prerequisites

- C++ compiler with C++17 support
- CMake (version 3.10 or higher)
- Boost libraries (system, thread)

### Build Steps

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the server
make

# Run the server (default port is 7777)
./bin/DwarfMMO_Server

# Run with a specific port
./bin/DwarfMMO_Server 8888
```

## Server Configuration

The server can be configured by editing the `server_config.txt` file or by passing command-line arguments:

- Default port: 7777
- Max clients: 100
- Tick rate: 20 updates per second
- Default world size: 500x500 tiles

## Network Protocol

The server uses a custom binary protocol for client-server communication:
- 4-byte packet length header
- Binary packet data

Packet types include:
- Connection management (request/accept)
- Player movement
- World modifications

## Architecture

The server is organized into the following components:

1. `Server` - Main server class that manages client connections and the game world
2. `ClientSession` - Handles individual client connections and communication
3. `World` - Maintains the game world state, including tiles and entities
4. `Entity/Player` - Base classes for game entities

## Next Steps

1. Implement persistent world storage
2. Add more game mechanics (resource gathering, building, etc.)
3. Implement Z-layers (vertical dimension)
4. Add more sophisticated world generation
5. Implement player authentication and accounts