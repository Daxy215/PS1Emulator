#include "TrackBuilder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

struct FileAudioData {
    uint32_t offset = 0;
    uint32_t size = 0;
};

static uint32_t parseCueIndexLba(const std::string& value) {
    int minutes = std::stoi(value.substr(0, 2));
    int seconds = std::stoi(value.substr(3, 2));
    int frames = std::stoi(value.substr(6, 2));
    
    return static_cast<uint32_t>((minutes * 60 * 75) + (seconds * 75) + frames);
}

static uint32_t readLe32(std::ifstream& file) {
    uint8_t bytes[4]{};
    file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    return static_cast<uint32_t>(bytes[0])
           | (static_cast<uint32_t>(bytes[1]) << 8)
           | (static_cast<uint32_t>(bytes[2]) << 16)
           | (static_cast<uint32_t>(bytes[3]) << 24);
}

static FileAudioData getFileAudioData(const std::string& path, bool waveFile) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error("Failed to open BIN file");
    
    const auto fullSize = static_cast<uint32_t>(file.tellg());
    if (!waveFile) {
        return {0, fullSize};
    }
    
    file.seekg(0, std::ios::beg);
    
    char riff[4]{};
    char wave[4]{};
    file.read(riff, sizeof(riff));
    readLe32(file);
    file.read(wave, sizeof(wave));
    
    if (std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE") {
        return {0, fullSize};
    }
    
    while (file.good()) {
        char chunkId[4]{};
        file.read(chunkId, sizeof(chunkId));
        if (file.gcount() != sizeof(chunkId)) {
            break;
        }
        
        uint32_t chunkSize = readLe32(file);
        uint32_t chunkStart = static_cast<uint32_t>(file.tellg());
        
        if (std::string(chunkId, 4) == "data") {
            return {chunkStart, chunkSize};
        }
        
        file.seekg(chunkSize + (chunkSize & 1), std::ios::cur);
    }
    
    return {0, fullSize};
}

std::vector<Track> TrackBuilder::parseFile(const std::string& filePath) {
	if(filePath.empty()) {
		return {};
	}
	
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
    
    std::string currentFile;
    std::string currentFileType;
    std::string line;
    while (std::getline(cueFile, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line.find("FILE") == 0) {
            size_t pos = line.find('"');
            size_t endPos = line.find('"', pos + 1);
            std::string fileName = line.substr(pos + 1, endPos - pos - 1);
            currentFile = (filePath.parent_path() / fileName).string();
            currentFileType = line.substr(endPos + 1);
            trim(currentFileType);
            continue;
        }
        
        if (line.find("TRACK") == 0 && !currentFile.empty()) {
            Track track;
            track.filePath = currentFile;
            track.type = line;
            
            size_t modePos = line.find("MODE");
            size_t audioPos = line.find("AUDIO");
            
            if (audioPos != std::string::npos) {
                track.mode = "AUDIO";
                track.modeType = 2352;
            } else if (modePos != std::string::npos) {
                track.mode = line.substr(modePos, 5);
                track.modeType = std::stoi(line.substr(modePos + 6));
            } else {
                continue;
            }
            
            const bool waveFile = currentFileType.find("WAVE") != std::string::npos
                                  || std::filesystem::path(track.filePath).extension() == ".wav";
            auto audioData = getFileAudioData(track.filePath, track.mode == "AUDIO" && waveFile);
            track.fileDataOffset = audioData.offset;
            track.sectorCount = static_cast<uint32_t>(audioData.size / track.modeType);
            
            std::cerr << "Track " << (tracks.size() + 1)
                      << " contains " << track.sectorCount
                      << " sectors (" << track.mode << ")\n";
            
            tracks.push_back(track);
            continue;
        }
        
        if (line.find("INDEX 01") == 0 && !tracks.empty()) {
            size_t pos = line.find_last_of(' ');
            if (pos != std::string::npos) {
                tracks.back().trackIndex = parseCueIndexLba(line.substr(pos + 1));
            }
        }
    }
    
    std::unordered_map<std::string, uint32_t> fileSectors;
    for (const auto& track : tracks) {
        if (fileSectors.find(track.filePath) != fileSectors.end()) {
            continue;
        }
        
        auto audioData = getFileAudioData(track.filePath, track.fileDataOffset != 0);
        fileSectors[track.filePath] = static_cast<uint32_t>(audioData.size / track.modeType);
    }
    
    for (size_t i = 0; i < tracks.size(); i++) {
        uint32_t end = fileSectors[tracks[i].filePath];
        for (size_t j = i + 1; j < tracks.size(); j++) {
            if (tracks[j].filePath == tracks[i].filePath) {
                end = tracks[j].trackIndex;
                break;
            }
        }
        
        tracks[i].sectorCount = end > tracks[i].trackIndex
                                ? end - tracks[i].trackIndex
                                : fileSectors[tracks[i].filePath];
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
