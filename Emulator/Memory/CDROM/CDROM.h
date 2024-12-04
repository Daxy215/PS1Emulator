#pragma once
#include <stdint.h>
#include <queue>

#include "Disk.h"

#include "../IRQ.h"

class CDROM {
public:
	CDROM();
	
	void step(uint32_t cycles);
	
	template<typename T>
	T load(uint32_t addr);
	
	template<typename T>
	void store(uint32_t addr, T val);
	
	// Testing
	void triggerInterrupt() {
		if((IE & IF) != 0) {
			// TODO; Check if this is correct?
			transmittingCommand = false;
			
			// Interrupt
			IRQ::trigger(IRQ::Interrupt::CDROM);
		}
	}
	
public:
	void swapDisk(const std::string& path);
	
	void decodeAndExecute(uint8_t command);
	void decodeAndExecuteSub();

private:
	// CDROM Commands
	void GetStat();
	void SetLoc();
	void SeekL();
	void GetID();
	
	// Intterupts
	void INT3();
	
private:
	uint8_t _index = 0;
	uint8_t _stats = 0;
	
	uint8_t IE = 0;
	uint8_t IF = 0;
	
	bool transmittingCommand = false;
	bool diskPresent = false;
	
	Disk _disk;
	
	Sector _readSector;
	Sector _sector;
	
	// TODO; Make size of 16
	// Rename to parameters
	std::queue<uint8_t> parameters;
	std::queue<uint8_t> responses;
	std::queue<uint8_t> interrupts;
};
