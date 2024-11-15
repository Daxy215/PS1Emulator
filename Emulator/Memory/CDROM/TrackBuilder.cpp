#include "TrackBuilder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

void TrackBuilder::parseFile(const std::string& filePath) {
	std::filesystem::path path = filePath;
	
	std::string extension = path.extension().string();
	
	if(extension._Equal(".cue")) {
		std::cerr << "Building a cue file\n";
		parseCueFile(filePath);
	} else if(extension._Equal(".bin")) {
		std::cerr << "Building a bin file\n";
		
	} else {
		throw std::runtime_error("Unsupported disk type");
	}
}

std::vector<std::vector<uint8_t>> TrackBuilder::parseCueFile(const std::string& path) {
	std::ifstream cueFile(path);
    std::vector<Track> entries;
    
    std::string line;
    while (std::getline(cueFile, line)) {
        if (line.find("FILE") == 0) {
            size_t pos = line.find('"');
            size_t endPos = line.rfind('"', pos + 1);
			
        	// TODO; This is wrong
            std::string fileName = line.substr(pos + 1, endPos - pos - 1);
            
            // Read .BIN file
        	// TODO; im too lazy to deal with this
            std::ifstream binFile("ROMS/Crash Bandicoot (USA)/Crash Bandicoot (USA).bin", std::ios::binary | std::ios::ate);
        	
            if (!binFile.is_open())
            	throw std::runtime_error("Failed to open BIN file");

        	size_t fileSize = binFile.tellg();
            binFile.seekg(0, std::ios::beg);
        	
        	std::vector<uint8_t> data{std::istreambuf_iterator<char>(binFile),
						   std::istreambuf_iterator<char>()};
        	
            // Parse .CUE entries
            while (std::getline(cueFile, line)) {
                if (line.empty() || line[0] == '#') continue;
                size_t spacePos = line.find(' ');
            	
                Track entry;
                entry.type = line.substr(0, spacePos);
                entry.track = std::stoi(line.substr(spacePos + 1, line.find(' ', spacePos + 1) - spacePos - 1));
                
                std::getline(cueFile, line);
                entry.mode = line;
                
                std::getline(cueFile, line);
                size_t commaPos = line.find(',');
                entry.start = std::stol(line.substr(0, commaPos));
                entry.end = std::stol(line.substr(commaPos + 1));
                
                entries.push_back(entry);
            }
            
            std::vector<std::vector<uint8_t>> sectors;
			
        	// MODE2/2352 for crash
        	// TODO; hardcoded for now
            uint32_t sectorSize = 2352;
            
            uint32_t currentSector = 0;
            for (const auto& entry : entries) {
                uint32_t sectorsCount = entry.end - entry.start + 1;
                sectorsCount = std::min(sectorsCount, static_cast<uint32_t>(data.size()) / sectorSize);
                
                for (uint32_t i = 0; i < sectorsCount; ++i) {
                    uint32_t start = currentSector * sectorSize;
                    uint32_t end = std::min(start + sectorSize, static_cast<uint32_t>(data.size()));
                    sectors.push_back(std::vector<uint8_t>(data.begin() + start, data.begin() + end));
                }
                
                currentSector += sectorsCount;
            }
            
            return sectors;
        }
    }
	
	// Build the bin file
	// For now I'm just doing it manually
	//parseBinFile("ROMS/Crash Bandicoot (USA)/Crash Bandicoot (USA).bin");
	
	return {};
}

void TrackBuilder::parseBinFile(const std::string& path) {
	std::ifstream binFile(path);
	if (!binFile.is_open()) {
		std::cerr << "Error: Could not find bin file." << '\n';
		return;
	}
	
	std::cerr << "";
}

void TrackBuilder::trim(std::string& str) {
	const std::string whitespace = " \t\n\r";
	str.erase(0, str.find_first_not_of(whitespace));
	str.erase(str.find_last_not_of(whitespace) + 1);
}
