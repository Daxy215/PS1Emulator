#include "Sector.h"

void Sector::set(const std::vector<uint8_t>& data) {
	pointer = 0;
	size = data.size();
	std::memcpy(buffer.data(), data.data(), size);
}

uint8_t& Sector::load8() {
	return buffer[pointer++];
}

int16_t& Sector::load16() {
	int16_t* data = reinterpret_cast<int16_t*>(&buffer[pointer]);
	pointer += 2;
	
	return *data;
}

std::vector<uint8_t> Sector::read() {
	std::vector<uint8_t> data(size);
	
	std::memcpy(data.data(), &buffer[pointer], size);
	pointer += size;
	
	return data;
}

std::vector<uint8_t> Sector::read(size_t size) {
	std::vector<uint8_t> data(size);
	
	std::memcpy(data.data(), &buffer[pointer], size * 4);
	pointer += size * 4;
	
	return data;
}

bool Sector::isEmpty() {
	return pointer >= size;
}

void Sector::empty() {
	pointer = 0;
	size = 0;
}
