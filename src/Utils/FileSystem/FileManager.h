﻿#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace Emulator {
	namespace Utils {
		class FileManager {
		public:
			static std::vector<uint8_t> loadFile(const std::string& path) {
				std::ifstream stream(path.c_str(), std::ios::binary | std::ios::ate);
				
				if (!stream.good()) {
					std::cerr << "Cannot read from file: " << path << '\n';
					return {};
				}
				
				auto fileSize = stream.tellg();
				stream.seekg(0, std::ios::beg);
				
				std::vector<uint8_t> exe(fileSize);
				
				if (!stream.read(reinterpret_cast<char*>(exe.data()), fileSize)) {
					std::cerr << "Error reading file!" << '\n';
					
					return {};
				}
				
				return exe;
			}
		};
	}
}
