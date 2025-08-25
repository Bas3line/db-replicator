#include "replicator.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <regex>
#include <iomanip>

FastCDCReplicator::FastCDCReplicator(const Config& cfg) : config(cfg) {}

FastCDCReplicator::~FastCDCReplicator() {
    stop();
}

bool FastCDCReplicator::initialize() {
    log("INFO", "Initializing CDC Replicator...");
    
    if (!setup_connections()) {
        log("ERROR", "Failed to setup connections");
        return false;
    }
    
    if (!setup_publication()) {
        log("ERROR", "Failed to setup publication");
        return false;
    }
    
    if (!setup_replication_slot()) {
        log("ERROR", "Failed to setup replication slot");
        return false;
    }
    
    replicate_schema();
    log("INFO", "CDC Replicator initialized successfully");
    return true;
}

bool FastCDCReplicator::setup_connections() {
    try {
        prod_conn = std::make_unique<pqxx::connection>(config.get_prod_dsn());
        backup_conn = std::make_unique<pqxx::connection>(config.get_backup_dsn());
        
        if (!prod_conn->is_open() || !backup_conn->is_open()) {
            return false;
        }
        
        log("INFO", "Database connections established");
        return true;
    } catch (const std::exception& e) {
        log("ERROR", "Connection failed: " + std::string(e.what()));
        return false;
    }
}

bool FastCDCReplicator::setup_publication() {
    try {
        pqxx::work tx(*prod_conn);
        
        auto result = tx.exec(
            "SELECT 1 FROM pg_publication WHERE pubname = " + tx.quote(config.publication_name)
        );
        
        if (result.empty()) {
            tx.exec("CREATE PUBLICATION " + config.publication_name + " FOR ALL TABLES");
            log("INFO", "Created publication: " + config.publication_name);
        }
        
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        log("ERROR", "Publication setup failed: " + std::string(e.what()));
        return false;
    }
}

bool FastCDCReplicator::setup_replication_slot() {
    try {
        pqxx::work tx(*prod_conn);
        
        auto result = tx.exec(
            "SELECT 1 FROM pg_replication_slots WHERE slot_name = " + tx.quote(config.replication_slot)
        );
        
        if (result.empty()) {
            tx.exec("SELECT pg_create_logical_replication_slot('" + 
                   config.replication_slot + "', 'test_decoding')");
            log("INFO", "Created replication slot: " + config.replication_slot);
        }
        
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        log("ERROR", "Replication slot setup failed: " + std::string(e.what()));
        return false;
    }
}

void FastCDCReplicator::replicate_schema() {
    try {
        pqxx::work backup_tx(*backup_conn);
        
        log("INFO", "Schema replication skipped - using existing schema");
        
        backup_tx.commit();
    } catch (const std::exception& e) {
        log("ERROR", "Schema replication failed: " + std::string(e.what()));
    }
}

void FastCDCReplicator::start_replication() {
    log("INFO", "Starting CDC replication...");
    
    while (running) {
        try {
            process_changes();
            std::this_thread::sleep_for(std::chrono::milliseconds(config.poll_interval_ms));
        } catch (const std::exception& e) {
            log("ERROR", "Replication error: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void FastCDCReplicator::process_changes() {
    pqxx::work prod_tx(*prod_conn);
    
    std::string query = "SELECT lsn, xid, data FROM pg_logical_slot_get_changes('" + 
                       config.replication_slot + "', NULL, " + std::to_string(config.batch_size) + ")";
    
    auto result = prod_tx.exec(query);
    
    if (result.empty()) {
        prod_tx.commit();
        return;
    }
    
    pqxx::work backup_tx(*backup_conn);
    int processed = 0;
    
    for (const auto& row : result) {
        std::string lsn = row[0].as<std::string>();
        std::string data = row[2].as<std::string>();
        
        execute_change(backup_tx, data);
        last_lsn = lsn;
        processed++;
    }
    
    backup_tx.commit();
    prod_tx.commit();
    
    if (processed > 0) {
        log("INFO", "Processed " + std::to_string(processed) + " changes");
    }
}

void FastCDCReplicator::execute_change(pqxx::work& tx, const std::string& change_data) {
    if (change_data.find("BEGIN") == 0 || change_data.find("COMMIT") == 0) {
        return;
    }
    
    try {
        if (change_data.find("table") != std::string::npos) {
            log("DEBUG", "Change: " + change_data.substr(0, 200));
            
            if (change_data.find("INSERT") != std::string::npos ||
                change_data.find("UPDATE") != std::string::npos ||
                change_data.find("DELETE") != std::string::npos) {
                log("INFO", "Processing: " + change_data.substr(0, 100));
            }
        }
    } catch (const std::exception& e) {
        log("ERROR", "Failed to execute change: " + std::string(e.what()));
    }
}

void FastCDCReplicator::stop() {
    running = false;
    log("INFO", "CDC Replicator stopped");
}

void FastCDCReplicator::log(const std::string& level, const std::string& message) {
    if (level == "DEBUG" && config.log_level != "DEBUG") return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
              << "] [" << level << "] " << message << std::endl;
}
