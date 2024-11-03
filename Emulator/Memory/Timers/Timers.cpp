#include "Timers.h"

Emulator::IO::Timers::Timers() {
	
}

void Emulator::IO::Timers::step(uint32_t cycles) {
	timers[0].step(cycles);
	timers[1].step(cycles);
	timers[2].step(cycles);
}

uint32_t Emulator::IO::Timers::load(uint32_t addr) {
	auto index = addr >> 4;
	
	Timer& timer = timers[index];
	
	switch (addr & 0xF) {
		case 0: {
			// Counter (R/W)
			return timer.counter;
			
			break;
		}
		
		case 4: {
			// Mode (R/W)
			return timer.getMode();
			
			break;
		}
		
		case 8: {
			// Target value (R/W)
			return timer.target;
			
			break;
		}
	}
	
	return 0;
}

void Emulator::IO::Timers::store(uint32_t addr, uint32_t val) {
	auto index = addr >> 4;
	
	Timer& timer = timers[index];
	
	switch (addr & 0xF) {
		case 0: {
			// Counter (R/W)
			timer.counter = static_cast<uint16_t>(val);
			
			break;
		}
		
		case 4: {
			// Mode (R/W)
			timer.setMode(static_cast<uint16_t>(val));
			
			break;
		}
		
		case 8: {
			// Target value (R/W)
			timer.target = static_cast<uint16_t>(val);
			
			break;
		}
	}
}
