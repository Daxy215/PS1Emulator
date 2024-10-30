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
	void parseFile(const std::string& path);
	void parseCueFile(const std::string& path);
	
private:
	void trim(std::string& str);

private:
	std::string binFilePath;
	std::vector<Track> tracks;
};
