#pragma once
#include <vector>

class Sector {
public:
	Sector(size_t size) : pointer(0), size(size) {
		buffer.reserve(size);
	}
	
	void set(const std::vector<uint8_t>& data);
	
	uint8_t& load8();
	int16_t& load16();
	
	std::vector<uint8_t> read();
	std::vector<uint8_t> read(size_t size);
	
	bool isEmpty();
	void empty();

private:
	size_t size;
	size_t pointer;
	std::vector<uint8_t> buffer;

public:
	static const int RAW_BUFFER = 2352;
	static const int XA_BUFFER = RAW_BUFFER * 16;
	
};
