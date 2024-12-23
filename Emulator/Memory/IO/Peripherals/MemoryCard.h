#pragma once
#include <cstdint>

class MemoryCard {
public:
	uint16_t load(uint32_t val);
	
private:
	bool _interrupt = false;
	int state = 0;
};
