#pragma once
#include <cstdint>

class COP0 {
public:
	uint32_t read(uint32_t reg);
	void write(uint32_t reg, uint32_t value);
	
	
};
