#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

#include "server/server.hpp"
#include "server/config.hpp"

// Global server instance for signal handling
ServerPtr g_server;

// Signal handler
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        // Load server configuration
        ServerConfig config;
        
        // Parse command line arguments
        if (argc > 1) {
            config.port = static_cast<uint16_t>(std::stoi(argv[1]));
        }
        
        std::cout << "Starting DwarfMMO Server on port " << config.port << std::endl;
        
        // Set up signal handling
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Create and start the server
        boost::asio::io_context ioContext;
        g_server = std::make_shared<Server>(ioContext, config);
        g_server->start();
        
        // Run the io_context in the main thread
        std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
        ioContext.run();
        
        std::cout << "Server stopped." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}