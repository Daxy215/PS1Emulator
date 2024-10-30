#pragma once
#include <string>

#include "Sector.h"
#include "TrackBuilder.h"

class Disk {
public:
	Disk();
	
	void load(const std::string& path);
	
private:
	Sector _sector;
	TrackBuilder _builder;
};
