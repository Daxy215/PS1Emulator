#pragma once
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <queue>

#include "Sector.h"

class CDROM {
public:
	CDROM() : _readSector(Sector::RAW_BUFFER), _sector(Sector::RAW_BUFFER) {
		
	}
	
	template<typename T>
	T load(uint32_t addr) {
		// TODO; Make a cycle function or tick?
		if((IF & IE) != 0) {
			transmittingCommand = false;
		}
		
		printf("CDROM LOAD %x\n", addr);
		std::cerr << "";
		
		if(addr == 0x1f801800) {
			T stat = 0;
			
			//7 BUSYSTS Command/parameter transmission busy  (1=Busy)
			stat |= transmittingCommand ? 1 : 0 << 7;
			
			// 6 DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
			stat |= 0 << 6;
			
			// 5 RSLRRDY Response fifo empty  (0=Empty) ;triggered after reading LAST byte
			stat |= 0 << 5;
			//stat |= !responses.empty() << 5;
			
			// 4 PRMWRDY Parameter fifo full  (0=Full)  ;triggered after writing 16 bytes	
			//stat |= paramaters.size() >= 16 << 4;
			stat |= 1 << 4;
			
			// 3 PRMEMPT Parameter fifo empty (1=Empty) ;triggered before writing 1st byte
			//stat |= paramaters.empty() << 3;
			stat |= 1 << 3;
			
			// 2 ADPBUSY XA-ADPCM fifo empty  (0=Empty) ;set when playing XA-ADPCM sound
			stat |= 0 << 2;
			
			// 0-1 Index   Port 1F801801h-1F801803h index (0..3 = Index0..Index3)   (R/W)
			stat |= _index;
			
			return stat;
		}
		
		return 0b11111111;
	}
	
	template<typename T>
	void store(uint32_t addr, T val) {
		// TODO; Make a cycle function or tick?
		if((IF & IE) != 0) {
			transmittingCommand = false;
		}
		
		printf("CDROM %x - %x\n", addr, val);
		std::cerr << "";
		
		if(addr == 0x1f801800) {
			// https://psx-spx.consoledev.net/cdromdrive/#1f801800h-indexstatus-register-bit0-1-rw-bit2-7-read-only
			// READ-ONLY
			_index = static_cast<uint8_t>(val & 0x3);
			
			return;
		} else if(addr == 0x1f801801) {
			switch (_index) {
				case 0: {
					// Command register
					printf("Command; %x", val);
					std::cerr << "";
					
					transmittingCommand = true;
					decodeAndExecute(val);
					
					break;
				}
			}
		} else if(addr == 0x1f801802) {
			switch (_index) {
				case 0: {
					printf("Paramater; %x\n", val);
					std::cerr << "";
					
					paramaters.push(val);
					
					break;
				}
				
				case 1: {
					IE = val & 0x1F;
					
					break;
				}
			}
		} else if(addr == 0x1f801803) {
			switch (_index) {
				case 0: {
					// 1F801803h.Index0 - Request Register(W)
					//0 - 4 0    Not used(should be zero)
					//5   SMEN Want Command Start Interrupt on Next Command(0 = No change, 1 = Yes)
					//6   BFWR...
					//7   BFRD Want Data(0 = No / Reset Data Fifo, 1 = Yes / Load Data Fifo)// 1F801803h.Index0 - Request Register(W)
					//0 - 4 0    Not used(should be zero)
					//5   SMEN Want Command Start Interrupt on Next Command(0 = No change, 1 = Yes)
					//6   BFWR...
					//7   BFRD Want Data(0 = No / Reset Data Fifo, 1 = Yes / Load Data Fifo)
					if((val & 0b10000000) != 0) {
						if(!_sector.isEmpty()) {
							_sector.set(_readSector.read());
						} else {
							_sector.empty();
						}
					}
					
					break;
				}
				
				case 1: {
					//IF &= ~(val & 0x1F);
					IF = val & 0x1F;
					
					if((val & 0x40) == 0x40) {
						for(int i = 0; i < paramaters.size(); i++)
							paramaters.pop();
					}
					
					break;
				}
			}
		}
	}
	
	void decodeAndExecute(uint8_t command);
	void decodeAndExecuteSub();

private:
	uint8_t _index = 0;
	uint8_t _stats = 0;
	
	uint8_t IE;
	uint8_t IF;

	bool transmittingCommand = false;
	
	Sector _readSector;
	Sector _sector;
	
	// TODO; Make size of 16
	std::queue<uint8_t> paramaters;
	std::queue<uint8_t> responses;
};
