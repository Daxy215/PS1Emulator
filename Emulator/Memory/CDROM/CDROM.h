#pragma once
#include <cstdint>
#include <cstdio>
#include <iostream>

class CDROM {
public:
	template<typename T>
	T load(uint32_t addr) {
		printf("CDROM LOAD %x\n", addr);
		std::cerr << "";
		
		return 0b11111111;
	}
	
	template<typename T>
	void store(uint32_t addr, T val) {
		printf("CDROM %x - %x\n", addr, val);
		std::cerr << "";
		
		if(addr == 0x1f801800) {
			// https://psx-spx.consoledev.net/cdromdrive/#1f801800h-indexstatus-register-bit0-1-rw-bit2-7-read-only
			// READ-ONLY
			index = static_cast<uint8_t>(val & 0x3);
			
			return;
		}
	}

private:
	uint8_t index = 0;
};
