#include "TrackBuilder.h"

#include <fstream>
#include <iostream>
#include <sstream>

void TrackBuilder::parseCueFile(const std::string& path) {
	std::ifstream cueFile(path);
	if (!cueFile.is_open()) {
		std::cerr << "Error: Could not open cue file." << '\n';
		return;
	}
	
	std::string line;
	while (std::getline(cueFile, line)) {
		trim(line);
		
		if (line.rfind("FILE", 0) == 0) {
			std::istringstream iss(line);
			std::string token;
			
			// Skip "FILE"
			iss >> token;
			iss >> binFilePath;
		} else if (line.rfind("TRACK", 0) == 0) {
			Track track;
			std::istringstream iss(line);
			std::string token;
			
			// Skip "TRACK"
			iss >> token;
			iss >> track.trackNumber;
			
			// The next line should be the INDEX
			std::getline(cueFile, line);
			if (line.rfind("INDEX", 0) == 0) {
				std::istringstream issIndex(line);
				
				std::string indexToken;
				
				// Skip "INDEX"
				issIndex >> indexToken;
				
				int indexNumber;
				issIndex >> indexNumber;
				if (indexNumber == 1) {
					// MM:SS:FF
					int minute, second, frame;
					char colon;
					
					issIndex >> minute >> colon >> second >> colon >> frame;
					track.startLBA = (minute * 60 + second) * 75;
				}
			}
			
			tracks.push_back(track);
		}
	}
	
	cueFile.close();
}

void TrackBuilder::trim(std::string& str) {
	const std::string whitespace = " \t\n\r";
	str.erase(0, str.find_first_not_of(whitespace));
	str.erase(str.find_last_not_of(whitespace) + 1);
}
