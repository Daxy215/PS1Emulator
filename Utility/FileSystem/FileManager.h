#pragma once
#include <string>
#include <vector>

class FileManager {
public:
	static std::vector<uint8_t> loadFile(const std::string& path);
};
