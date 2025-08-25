#include <iostream>
#include <signal.h>
#include <memory>
#include "config.hpp"
#include "replicator.hpp"

std::unique_ptr<FastCDCReplicator> replicator;

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    if (replicator) {
        replicator->stop();
        replicator.reset();
    }
    exit(0);
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        std::cout << "Starting PostgreSQL CDC Replicator..." << std::endl;
        
        Config config;
        std::cout << "Production DB: " << config.prod_host << ":" << config.prod_port << std::endl;
        std::cout << "Backup DB: " << config.backup_host << ":" << config.backup_port << std::endl;
        
        replicator = std::make_unique<FastCDCReplicator>(config);
        
        if (!replicator->initialize()) {
            std::cerr << "Failed to initialize replicator" << std::endl;
            return 1;
        }
        
        replicator->start_replication();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}