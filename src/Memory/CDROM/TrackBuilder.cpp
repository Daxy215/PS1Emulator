#include "TrackBuilder.h"

#include <filesystem>
#include <fstream>
#include <iostream>

std::vector<Track> TrackBuilder::parseFile(const std::string& filePath) {
	std::filesystem::path path = filePath;
	
	std::string extension = path.extension().string();
	
	if(extension == (".cue")) {
		std::cerr << "Building a cue file\n";
		return parseCueFile(filePath);
	} else if(extension == (".bin")) {
		std::cerr << "Building a bin file\n";
	} else {
		throw std::runtime_error("Unsupported disk type");
	}
	
	return {};
}

std::vector<Track> TrackBuilder::parseCueFile(const std::string& path) {
	std::filesystem::path filePath = path;

	std::ifstream cueFile(path);
    std::vector<Track> tracks;  // NOLINT(clang-diagnostic-shadow)

	if(!cueFile) {
		std::cerr << "Error; Couldn't find disk with the path of: " << path << "\n";

		// Print current directory for debugging
		std::filesystem::path current_path = std::filesystem::current_path();

		std::cerr << "Current path: " << current_path << std::endl;
		
		throw std::runtime_error("Couldn't find disk through the provided path.");

		return {};
	}
    
    std::string line;
    while (std::getline(cueFile, line)) {
        if (line.find("FILE") == 0) {
            size_t pos = line.find('"');
            size_t endPos = line.find('"', pos + 1);
			
        	// TODO; This is wrong
            std::string fileName = line.substr(pos + 1, endPos - pos - 1);
			
        	// TODO; im too lazy to deal with this
        	//auto path = "ROMS/Crash Bandicoot (Europe, Australia)/Crash Bandicoot (Europe, Australia).bin";
			auto fullPath = filePath.parent_path() / fileName;
        	
            // Read .BIN file
            std::ifstream binFile(fullPath, std::ios::binary | std::ios::ate);
        	
            if (!binFile.is_open())
            	throw std::runtime_error("Failed to open BIN file");
			
        	size_t fileSize = binFile.tellg();
            binFile.seekg(0, std::ios::beg);
        	
            // Parse .CUE entries
            while (std::getline(cueFile, line)) {
                if (line.empty() || line[0] == '#') continue;
				
            	// "  TRACK 01 MODE2/2352"
                size_t spacePos = line.find("MODE");
				
            	// TODO; im too lazy to handle mulitple tracks
            	if (spacePos == std::string::npos) {
            		continue;
            	}
            	
                Track track;
            	
            	track.filePath = fullPath.string();
                track.type = line.substr(0, spacePos);
                track.mode = line.substr(spacePos, 5);
            	
                track.modeType = std::stoi(line.substr(spacePos + 6)); // +4 -> Skip "Mode2/"
				
            	// Skip index line for now..
                std::getline(cueFile, line);
                
                tracks.push_back(track);
            }
            
        	// MODE2/2352 for crash
        	
            uint32_t currentSector = 0;
            for (auto& track : tracks) {
				uint32_t sectorSize = track.modeType;
            	
            	track.sectorCount = (static_cast<uint32_t>(fileSize) / sectorSize);
				
            	std::cerr << "File contains " << std::to_string(track.sectorCount) << " sectors\n";
                
                currentSector += track.sectorCount;
            }
        }
    }
	
	return tracks;
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
