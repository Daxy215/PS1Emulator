#include "Disk.h"

#include <iostream>
#include <memory>
#include <cstdio>

#include "CDROM.h"

Disk::Disk() = default;

std::vector<uint8_t> Disk::read(Location location) {
	auto buffer = std::vector<uint8_t>(Sector::RAW_BUFFER, 0);
	
	static auto f = fopen(tracks[0].filePath.c_str(), "rb");
	if (!f) {
		std::printf("Unable to load file %s\n", tracks[0].filePath.c_str());
		return {};
	}
	
	//auto file = files[track.filename].get();
	auto seek = location;
	seek.seconds -= 2;
	
	// 0 -> track num
	long offset = 0 + seek.toLba() * Sector::RAW_BUFFER;
	
	/*if (offset < 0) {  // Pregap
		return std::make_pair(buffer, track.type);
	}*/
	
	fseek(f, offset, SEEK_SET);
	fread(buffer.data(), Sector::RAW_BUFFER, 1, f);
	
	return buffer;
}

void Disk::set(const std::string& path) {
	tracks = _builder.parseFile(path);
}
