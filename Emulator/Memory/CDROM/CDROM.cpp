#include "CDROM.h"

CDROM::CDROM() : _readSector(Sector::RAW_BUFFER), _sector(Sector::RAW_BUFFER) {
	
}

void CDROM::step(uint32_t cycles) {
	if(!interrupts.empty() && IF == 0) {
		IF |= interrupts.front();
		interrupts.pop();
	}
	
	triggerInterrupt();
}

template <>
uint32_t CDROM::load<uint32_t>(uint32_t addr) {
	if(addr == 0x1f801800) {
		uint32_t stat = 0;
		
		//7 BUSYSTS Command/parameter transmission busy  (1=Busy)
		stat |= (transmittingCommand ? 1 : 0) << 7;
		
		// 6 DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
		stat |= 0 << 6;
		
		// 5 RSLRRDY Response fifo empty  (0=Empty) ;triggered after reading LAST byte
		stat |= (responses.empty() ? 0 : 1) << 5;
		
		// 4 PRMWRDY Parameter fifo full  (0=Full)  ;triggered after writing 16 bytes	
		stat |= (parameters.size() >= 16 ? 0 : 1) << 4;
		
		// 3 PRMEMPT Parameter fifo empty (1=Empty) ;triggered before writing 1st byte
		stat |= (parameters.empty() ? 1 : 0) << 3;
		
		// 2 ADPBUSY XA-ADPCM fifo empty  (0=Empty) ;set when playing XA-ADPCM sound
		stat |= 0 << 2;
		
		// 0-1 Index   Port 1F801801h-1F801803h index (0..3 = Index0..Index3)   (R/W)
		stat |= (_index) << 0;
		
		return stat;
	} else if(addr == 0x1f801801) {
		auto response = responses.front();
		responses.pop();
		
		return response;
	} else if(addr == 0x1f801803) {
		switch (_index) {
		case 0: return IE | 0xE0;
		case 1: return IF | 0xE0;
		case 2: return IE; // Mirrored
		case 3: return IF; // Mirrored
		}
	}
	
	return 0b11111111;
}

template <>
void CDROM::store<uint32_t>(uint32_t addr, uint32_t val) {
	// Just cheating for now
	triggerInterrupt();
	
	if(addr == 0x1f801800) {
		// https://psx-spx.consoledev.net/cdromdrive/#1f801800h-indexstatus-register-bit0-1-rw-bit2-7-read-only
		// READ-ONLY
		_index = static_cast<uint8_t>(val & 0x3);
		
		return;
	} else if(addr == 0x1f801801) {
		switch (_index) {
			case 0: {
				// Command register
				transmittingCommand = true;
				decodeAndExecute(val);
				
				break;
			}
		}
	} else if(addr == 0x1f801802) {
		switch (_index) {
			case 0: {
				parameters.push(val);
				
				break;
			}
			
			case 1: {
				IE = val & 0x1F;
				triggerInterrupt();
				
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
				if((val & 0b1000000) != 0) {
					if(!_sector.isEmpty()) {
						_sector.set(_readSector.read());
					} else {
						_sector.empty();
					}
				}
				
				break;
			}
			
			case 1: {
				IF &= ~(val & 0x1F);
				//IF = val & 0x1F;
				triggerInterrupt();
				
				if((val & 0x40) == 0x40) {
					for(int i = 0; i < parameters.size(); i++)
						parameters.pop();
				}
				
				break;
			}
		}
	}
}

void CDROM::swapDisk(const std::string& path) {
	_disk.load(path);
}

void CDROM::decodeAndExecute(uint8_t command) {
	if(command == 0x1) {
		//   01h Getstat      -               INT3(stat)
		
		// TODO; Return stat register:
		
		/**
		 *  7  Play          Playing CD-DA         ;\only ONE of these bits can be set
		 * 6  Seek          Seeking               ; at a time (ie. Read/Play won't get
		 * 5  Read          Reading data sectors  ;/set until after Seek completion)
		 * 4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
		 * 3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
		 * 2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
		 * 1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
		 * 0  Error         Invalid Command/parameters (followed by Error Byte)
		 */
		uint8_t STAT = 0;
		
		// Lid is closed
		STAT = (STAT & (~0x18));
		STAT |= 0x2;
		
 		responses.push(STAT);
		
		// INT3
		interrupts.push(3);
	} else if(command == 0x19) {
		//   19h Test         sub_function    depends on sub_function (see below)
		decodeAndExecuteSub();
	}
}

void CDROM::decodeAndExecuteSub() {
	uint8_t command = parameters.front();
	parameters.pop();
	
	if(command == 0x20) {
		//   20h      -   INT3(yy,mm,dd,ver) ;Get cdrom BIOS date/version (yy,mm,dd,ver)
		
		// 95h,05h,16h,C1h  ;PSX (LATE-PU-8)          16 May 1995, version vC1 (a)
		// Idk what any of this shit means
		responses.push(0x95);
		responses.push(0x05);
		responses.push(0x16);
		responses.push(0xC1);
		
		// INT3
		interrupts.push(3);
	}
}
