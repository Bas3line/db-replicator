#pragma once
#include <string>
#include <cstdlib>
#include <stdexcept>

struct Config {
    std::string prod_host = getenv_or("PROD_DB_HOST", "enigma-postgres");
    std::string prod_port = getenv_or("PROD_DB_PORT", "5432");
    std::string prod_db = getenv_or("PROD_DB_NAME", "enigma");
    std::string prod_user = getenv_or("PROD_DB_USER", "enigma");
    std::string prod_pass = getenv_required("PROD_DB_PASSWORD");
    
    std::string backup_host = getenv_or("BACKUP_DB_HOST", "sm-backup");
    std::string backup_port = getenv_or("BACKUP_DB_PORT", "5432");
    std::string backup_db = getenv_or("BACKUP_DB_NAME", "enigma");
    std::string backup_user = getenv_or("BACKUP_DB_USER", "enigma");
    std::string backup_pass = getenv_required("BACKUP_DB_PASSWORD");
    
    std::string replication_slot = getenv_or("REPLICATION_SLOT", "cdc_slot");
    std::string publication_name = getenv_or("PUBLICATION_NAME", "db_changes");
    int batch_size = std::stoi(getenv_or("BATCH_SIZE", "1000"));
    int poll_interval_ms = std::stoi(getenv_or("POLL_INTERVAL_MS", "10"));
    std::string log_level = getenv_or("LOG_LEVEL", "INFO");
    
    std::string get_prod_dsn() const {
        return "host=" + prod_host + " port=" + prod_port + 
               " dbname=" + prod_db + " user=" + prod_user + 
               " password=" + prod_pass + " connect_timeout=10";
    }
    
    std::string get_backup_dsn() const {
        return "host=" + backup_host + " port=" + backup_port + 
               " dbname=" + backup_db + " user=" + backup_user + 
               " password=" + backup_pass + " connect_timeout=10";
    }

private:
    static std::string getenv_or(const char* name, const std::string& default_val) {
        const char* val = std::getenv(name);
        return val ? std::string(val) : default_val;
    }
    
    static std::string getenv_required(const char* name) {
        const char* val = std::getenv(name);
        if (!val) {
            throw std::runtime_error(std::string("Required environment variable not set: ") + name);
        }
        return std::string(val);
    }
};
