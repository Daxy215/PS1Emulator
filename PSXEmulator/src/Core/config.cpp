#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

Config& Config::Get() {
    static Config instance;
    return instance;
}

std::string Config::GetConfigPath() {
    auto base = std::filesystem::path(std::getenv("HOME"));
    return (base / ".ide_config.json").string();
}

void Config::Load() {
    std::ifstream file(GetConfigPath());
    if (!file.is_open()) {
        return;
    }
    
    json j;
    file >> j;
    
    
}

void Config::Save() {
    json j;
    
    
    std::ofstream file(GetConfigPath());
    file << j.dump(4);
}
