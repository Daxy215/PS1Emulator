#include "Disk.h"

#include <iostream>
#include <memory>
#include <cstdio>

#include "CDROM.h"

Disk::Disk() = default;

std::vector<uint8_t> Disk::read(Location location) {
	auto buffer = std::vector<uint8_t>(Sector::RAW_BUFFER, 0);
	
	int pos = getTrackPosition(location);
	
	if(pos == -1) {
		printf("");
	}
	
	static auto f = fopen(tracks[0].filePath.c_str(), "rb");
	if (!f) {
		std::printf("Unable to load file %s\n", tracks[0].filePath.c_str());
		return {};
	}
	
	auto seek = location - (getTrackBegin(pos) + tracks[pos].pregap);
	
	// 0 -> track num
	long offset = 0 + seek.toLba() * Sector::RAW_BUFFER;
	
	fseek(f, offset, SEEK_SET);
	fread(buffer.data(), Sector::RAW_BUFFER, 1, f);
	
	return buffer;
}

void Disk::set(const std::string& path) {
	tracks = _builder.parseFile(path);
}

int Disk::getTrackPosition(Location loc) {
	for(int i = 0; i < tracks.size(); i++) {
		auto begin = getTrackBegin(i);
		auto end = getTrackLength(i);
		
		if(loc >= begin && loc < begin + end) {
			return i;
		}
	}
	
	return -1;
}

Location Disk::getTrackBegin(int track) {
	size_t total = 75 * 2;
	for (int i = 0; i < track; i++) {
		total += tracks[i].pregap.toLba() + tracks[i].sectorCount;
	}
	return Location::fromLBA(total);
}

Location Disk::getTrackLength(int track) {
	return Location::fromLBA(tracks[track].pregap.toLba() + tracks[track].sectorCount);
}
