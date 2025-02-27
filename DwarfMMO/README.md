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

## Next Steps

- Implement server side logic
- Add multiplayer functionality
- Implement world modification
- Add Z-layers for height
- Implement game mechanics

## Controls

- WASD or Arrow keys to move the player character
- ESC to exit the game

## Network Protocol

The game uses a custom binary protocol for client-server communication. Packets include:
- Connection management (request/accept)
- Player position updates
- World modifications

## License

This project is licensed under the MIT License - see the LICENSE file for details.