﻿#pragma once

#include <stdint.h>
#include <queue>
#include <cstring>

#include "Disk.h"

#include "../IRQ.h"

class CDROM {
	union Stats {
		enum class Mode { None, Reading, Seeking, Playing };
		
		struct {
			uint8_t error : 1;
			uint8_t motor : 1;
			uint8_t seekError : 1;
			uint8_t idError : 1;
			uint8_t shellOpen : 1;
			uint8_t read : 1;
			uint8_t seek : 1;
			uint8_t play : 1;
		};
		
		uint8_t _reg;
		
		void setMode(Mode mode) {
			error = seekError = idError = false;
			read = seek = play = false;
			motor = true;
			
			if (mode == Mode::Reading) {
				read = true;
			} else if (mode == Mode::Seeking) {
				seek = true;
			} else if (mode == Mode::Playing) {
				play = true;
			}
		}
		
		void setShell(bool opened) {
			shellOpen = opened;
			
			setMode(Mode::None);
			
			if (opened) {
				motor = false;
			}
		}
		
		bool getShell() const { return shellOpen; }
		
		Stats() : _reg(0) {}
	};
	
	union Mode {
		struct {
			uint8_t cdda       : 1; // (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
			uint8_t autoPause  : 1; // (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
			uint8_t report     : 1; // (0=Off, 1=Enable Report-Interrupts for Audio Play)
			uint8_t xaFilter   : 1; // (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
			uint8_t ignoreBit  : 1; // (0=Normal, 1=Ignore Sector Size and Setloc position)
			uint8_t sectorSize : 1; // (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
			uint8_t xaAdpcm    : 1; // (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
			uint8_t speed      : 1; // (0=Normal speed, 1=Double speed)
		};
		
		uint8_t _reg;
		
		Mode() : _reg(0) {}
		Mode(uint8_t reg) : _reg(reg) {}
	};
	
	struct Interrupt {
		uint8_t _interrupt;
		std::queue<uint8_t> responses;
		
		int32_t delay;
		bool ack = false;
		
		Interrupt() = default;
		
		Interrupt(uint8_t interrupt, uint32_t delay)
			: _interrupt(interrupt),
			  delay(delay) {
			
		}
	};
	
public:
	CDROM();
	
	void step(uint32_t cycles);
	
	void handleSector();
	
	template<typename T>
	T load(uint32_t addr);
	
	template<typename T>
	void store(uint32_t addr, T val);
	
	uint8_t readByte();
	
public:
	void swapDisk(const std::string& path);
	
	void decodeAndExecute(uint8_t command);
	void decodeAndExecuteSub();

private:
	bool isEmpty();	
	
private:
	// CDROM Commands
	void GetStat();
	void SetLoc();
	void ReadN();
	void Stop();
	void Pause();
	void SetMode();
	void Init();
	void SeekL();
	void GetID();
	void ReadS();
	
private:
	// Interrupts
	void INT2();
	void INT3();
	
	void INT(uint8_t in, uint32_t delay = 50000) {
		interrupts.emplace(in, delay);
	}
	
	void addResponse(uint8_t response) {
		if(interrupts.empty()) {
			printf("");
			return;
		}
		
		if(interrupts.back().responses.size() >= 16) {
			return;
		}
		
		interrupts.back().responses.push(response);
	}
	
private:
	uint8_t getParamater();
	
private:
	uint8_t _index = 0;
	
	uint16_t IE = 0;
	//uint8_t IF = 0;
	
	int32_t busyFor = 0;
	uint32_t cycles = 0;
	
private:
	/**
	 * After copying a bunch of shit from Avocado because it's been weeks...
	 * THIS WAS THE ISSUE. I am literally out of words... ;-;
	 */
	int seekLocation;
	int readLocation;
	
	bool transmittingCommand = false;
	bool diskPresent = false;
	
	bool isBufferEmpty = false;
	
	bool mute = false;
	
private:
	Stats _stats;
	Mode mode;

private:
	Disk _disk;
	Sector _readSector;
	Sector _sector;
	
private:
	// TODO; Make size of 16
	// Rename to parameters
	std::queue<uint8_t> parameters;
	
	//std::queue<uint8_t> responses;
	std::queue<Interrupt> interrupts;
};
