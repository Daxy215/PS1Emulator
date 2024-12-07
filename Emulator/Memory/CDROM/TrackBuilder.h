#pragma once
#include <string>
#include <vector>

struct Track {
	std::string filePath;
	std::string type;
	uint32_t trackIndex;
	
	std::string mode;
	uint32_t modeType;
	
	uint32_t sectorCount;
	
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
