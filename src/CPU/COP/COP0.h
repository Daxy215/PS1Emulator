#pragma once
#include <cstdint>

class COP0 {
public:
	COP0();
	
	void reset();
	
public:
	uint32_t bpc = 0;
	uint32_t bda = 0;
	uint32_t dcic = 0;
	uint32_t bdam = 0;
	uint32_t bpcm = 0;

	// Cop0; register 8; BadVaddr - Bad Virtual Address (R)
	uint32_t badVaddr = 0;
    
	// Cop0; register 12; Status Register
	uint32_t sr = 0;
    
	// Cop0; register 13; Cause Register
	static uint32_t cause;
    
	// Cop0; register 14; EPC
	uint32_t epc = 0;
};
