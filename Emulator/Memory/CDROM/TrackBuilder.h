#pragma once
#include <string>
#include <vector>

struct Track {
	int trackNumber;
	
	// Logical Block Addressing
	int startLBA;
};

class TrackBuilder {
public:
	void parseCueFile(const std::string& path);
	void trim(std::string& str);
	
private:
	std::string binFilePath;
	std::vector<Track> tracks;
};
