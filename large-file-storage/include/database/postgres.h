#pragma once

#include <pqxx/pqxx>

#include <memory>
#include <string>

class Postgres {
public:

    explicit Postgres(
        const std::string& connectionString
    );

    pqxx::connection& connection();

private:

    std::unique_ptr<pqxx::connection> connection_;
};