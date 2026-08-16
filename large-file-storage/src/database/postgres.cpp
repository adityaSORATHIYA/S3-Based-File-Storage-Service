#include "database/postgres.h"

#include <stdexcept>

Postgres::Postgres(
    const std::string& connectionString
) {
    try {

        connection_ =
            std::make_unique<pqxx::connection>(
                connectionString
            );

        if (!connection_->is_open()) {
            throw std::runtime_error(
                "Failed to open PostgreSQL connection"
            );
        }

    }
    catch (const pqxx::broken_connection& e) {

        throw std::runtime_error(
            std::string("PostgreSQL connection failed: ")
            + e.what()
        );
    }
}


pqxx::connection& Postgres::connection() {

    return *connection_;
}