# Dwarf MMO

A dwarf-fortress style massively multiplayer online game where each player controls a single unit in a contiguous, modifiable world.

## Project Structure

This project uses a C++ client from scratch with the following dependencies:
- SDL2 for window management and rendering
- Boost for networking (Asio)
- CMake for build configuration

## Building the Project

### Prerequisites

You need to have the following installed:
- C++ compiler with C++17 support (GCC, Clang, or MSVC)
- CMake (version 3.10 or higher)
- SDL2 development libraries
- Boost development libraries (system and thread components)

### Linux/macOS

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
make

# Run the game
./DwarfMMO
```

### Windows with Visual Studio

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake -G "Visual Studio 17 2022" ..

# Open the Visual Studio solution
start DwarfMMO.sln
```

Or use CMake GUI to configure and generate the project files.

## Current Features

- Simple window creation with SDL2
- Basic ASCII rendering system
- Entity/player movement with collision detection
- Simple world generation
- Networking framework with packet serialization/deserialization
- Multiplayer functionality with player synchronization
- World modification (place and remove walls)
- Graceful handling of server disconnections
- Random player name generation for testing

## Next Steps

- Add more interactive world elements
- Add resource gathering mechanics
- Implement crafting system
- Add Z-layers for height
- Implement chat functionality
- Add authentication and user accounts
- Implement persistent player data

## Controls

- WASD or Arrow keys to move the player character
- E to place a green wall at your current location
- R to remove a wall at your current location
- ESC to exit the game

## Recent Improvements

- **Multiplayer Synchronization**: Players can now see each other in real-time and observe world modifications made by others
- **Crash Resilience**: Client gracefully handles server disconnections and shutdowns
- **User Experience**: Random player names with timeout for faster testing
- **Wall Placement**: Players can place and remove walls which synchronize across all clients
- **Distance Restriction**: Wall placement is limited to 5 tiles from player's position

## Network Protocol

The game uses a custom binary protocol for client-server communication. Packets include:
- Connection management (request/accept)
- Player position updates
- World modifications

## License

This project is licensed under the MIT License - see the LICENSE file for details.