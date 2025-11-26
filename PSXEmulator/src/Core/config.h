#pragma once
#include <string>

class Config {
    public:
        static Config& Get();
        
        void Load();
        void Save();
        
    private:
        Config() = default;
        [[nodiscard]] static std::string GetConfigPath() ;
        
    public:
        std::vector<>
};
