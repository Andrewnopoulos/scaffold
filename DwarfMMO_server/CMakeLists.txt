cmake_minimum_required(VERSION 3.10)
project(DwarfMMO_Server VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost packages
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
file(GLOB_RECURSE SERVER_SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Create server executable
add_executable(${PROJECT_NAME} ${SERVER_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} pthread)  # For Linux/Unix systems

# Add compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy config files if needed
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy
#     ${CMAKE_SOURCE_DIR}/config/server_config.json $<TARGET_FILE_DIR:${PROJECT_NAME}>/server_config.json
# )