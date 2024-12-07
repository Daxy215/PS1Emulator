#pragma once
#include <string>

#include "Sector.h"
#include "TrackBuilder.h"

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

class Location;

class Disk {
public:
	Disk();
	
	std::vector<uint8_t> read(Location location);
	
	void set(const std::string& path);
	
private:
	TrackBuilder _builder;
	
	std::vector<Track> tracks;
};
