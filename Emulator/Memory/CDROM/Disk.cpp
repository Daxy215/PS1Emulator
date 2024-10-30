#include "Disk.h"

Disk::Disk() : _sector(Sector::RAW_BUFFER) {
	
}

void Disk::load(const std::string& path) {
	_builder.parseFile(path);
}
