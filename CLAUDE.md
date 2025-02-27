# DwarfMMO Project Guidelines

## Build Commands
### Client
```bash
cd DwarfMMO && mkdir -p build && cd build && cmake .. && make
./DwarfMMO  # Run client
```

### Server
```bash
cd DwarfMMO_server && mkdir -p build && cd build && cmake .. && make
./bin/DwarfMMO_Server  # Default port 7777
./bin/DwarfMMO_Server 8888  # Custom port
```

## Code Style Guidelines
- **Classes**: PascalCase (Entity, Player, Renderer)
- **Methods**: camelCase (getPosition, updateWorld)
- **Members**: m_camelCase (m_position, m_isActive)
- **Constants**: ALL_CAPS
- **Files**: .hpp for headers, .cpp for implementation
- **Error handling**: Boost error codes for network, exceptions sparingly
- **Memory**: Smart pointers (std::shared_ptr, std::unique_ptr)
- **Dependencies**: Forward declarations to minimize includes
- **Base classes**: Virtual destructors when inheritance is used
- **Project structure**: Logical separation (client, server, game, network)