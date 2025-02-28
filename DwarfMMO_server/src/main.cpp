#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <boost/asio.hpp>

#include "server/server.hpp"
#include "server/config.hpp"

// Global server instance for signal handling
ServerPtr g_server;

// Flags to track server state
std::atomic<bool> g_shutdownRequested(false);

// Flags to track shutdown process
std::atomic<bool> g_shutdownInProgress(false);
std::atomic<int> g_signalCount(0);

// Signal handler
void signalHandler(int signal) {
    // Count how many times we've received signals
    int currentCount = ++g_signalCount;
    
    // First signal - initiate clean shutdown
    if (currentCount == 1) {
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
        g_shutdownRequested.store(true);
        g_shutdownInProgress.store(true);
        
        // Stop the server
        if (g_server) {
            g_server->stop();
        }
    }
    // Second signal - still shutting down, be patient
    else if (currentCount == 2) {
        std::cout << "Shutdown already in progress, please wait..." << std::endl;
    }
    // Third or more signals - force exit
    else {
        std::cout << "Forcing immediate exit after " << currentCount << " signals" << std::endl;
        std::exit(1);
    }
    
    // Exit directly for SIGTERM
    if (signal == SIGTERM) {
        std::exit(0);
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
        
        // Run the io_context in the main thread with work guard to prevent early exit
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard = 
            boost::asio::make_work_guard(ioContext);
        
        std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
        
        // Run IO context in a separate thread
        std::thread ioThread([&ioContext]() {
            try {
                ioContext.run();
            } catch (const std::exception& e) {
                std::cerr << "IO context error: " << e.what() << std::endl;
            }
        });
        
        // Wait for shutdown signal in main thread
        while (!g_shutdownRequested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Clean shutdown sequence
        std::cout << "Initiating clean shutdown..." << std::endl;
        
        // Cancel the work guard to allow io_context to exit when all handlers complete
        workGuard.reset();
        
        // Wait for IO thread to finish
        if (ioThread.joinable()) {
            ioThread.join();
        }
        
        // Final cleanup
        g_server.reset();
        
        std::cout << "Server stopped cleanly." << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}