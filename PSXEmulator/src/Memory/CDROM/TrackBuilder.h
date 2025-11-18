#pragma once

#include <string>
#include <vector>

#include "Location.h"

struct Track {
	std::string filePath;
	std::string type;
	
	uint32_t trackIndex = 0;
	
	std::string mode;
	uint32_t modeType;
	
	uint32_t sectorCount;
	
	Location pregap;
	
	// TODO; Handle index
	//uint32_t start;
	//uint32_t end;
};

class TrackBuilder {
public:
	std::vector<Track> parseFile(const std::string& path);
	
	std::vector<Track> parseCueFile(const std::string& path);
	void parseBinFile(const std::string& path);
	
private:
	void trim(std::string& str);
	
private:
	std::vector<Track> tracks;
};
