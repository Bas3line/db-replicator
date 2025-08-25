#pragma once
#include <pqxx/pqxx>
#include <atomic>
#include <memory>
#include <string>
#include "config.hpp"

class FastCDCReplicator {
private:
    Config config;
    std::unique_ptr<pqxx::connection> prod_conn;
    std::unique_ptr<pqxx::connection> backup_conn;
    std::atomic<bool> running{true};
    std::string last_lsn;

public:
    explicit FastCDCReplicator(const Config& cfg);
    ~FastCDCReplicator();
    
    bool initialize();
    void start_replication();
    void stop();

private:
    bool setup_connections();
    bool setup_replication_slot();
    bool setup_publication();
    void replicate_schema();
    void process_changes();
    void execute_change(pqxx::work& tx, const std::string& change_data);
    void log(const std::string& level, const std::string& message);
};
