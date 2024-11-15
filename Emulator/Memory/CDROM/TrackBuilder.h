#pragma once
#include <string>
#include <vector>

struct Track {
	/*int trackNumber;
	
	// Logical Block Addressing
	int startLBA;
	
	std::string binFilePath;
	std::string type;*/
	
	std::string type;
	uint32_t track;
	std::string mode;
	uint32_t start;
	uint32_t end;
};

class TrackBuilder {
public:
	void parseFile(const std::string& path);
	
	std::vector<std::vector<uint8_t>> parseCueFile(const std::string& path);
	void parseBinFile(const std::string& path);
	
private:
	void trim(std::string& str);
	
private:
	std::vector<Track> tracks;
};
