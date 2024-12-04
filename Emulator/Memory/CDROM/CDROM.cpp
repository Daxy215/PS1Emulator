#include "CDROM.h"

#include <array>
#include <iostream>

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
	printf("CDROM Load %x\n", addr);
	std::cerr << "";
	
	if(addr == 0x1f801800) {
		uint32_t stat = 0;
		
		//7 BUSYSTS Command/parameter transmission busy  (1=Busy)
		stat |= (transmittingCommand ? 1 : 0) << 7;
		
		// 6 DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
		// TODO; Using this for testing for now
		stat |= (diskPresent ? 1 : 0) << 6;
		
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
		default:
			printf("");
			break;
		}
	}
	
	return 0b11111111;
}

template <>
void CDROM::store<uint32_t>(uint32_t addr, uint32_t val) {
	printf("CDROM write %x %x\n", addr, val);
	std::cerr << "";
	
	// Just cheating for now
	//triggerInterrupt();
	
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
		default:
			printf("");
			break;
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
				if(val & 0x80) {
					// Load fifo
					if(_sector.isEmpty()) {
						_sector.set(_readSector.read());
					}
				} else {
					_sector.empty();
				}
				
				break;
			}
			
			case 1: {
				IF &= ~(val & 0x1F);
				//IF = val & 0x1F;
				triggerInterrupt();
				
				if((val & 0x40) == 0x40) {
					while(!parameters.empty())
						parameters.pop();
				}
				
				break;
			}
		default:
			printf("");
			break;
		}
	} else {
		printf("");
	}
}

void CDROM::swapDisk(const std::string& path) {
	_disk.load(path);
}

void CDROM::decodeAndExecute(uint8_t command) {
	if(command == 0x1) {
		//   01h Getstat      -               INT3(stat)
		GetStat();
	} else if(command == 0x2) {
		// Setloc - Command 02h,amm,ass,asect --> INT3(stat)
		SetLoc();
	} else if(command == 0x15) {
	 	// SeekL - Command 15h --> INT3(stat) --> INT2(stat)
		SeekL();
	} else if(command == 0x19) {
		//   19h Test         sub_function    depends on sub_function (see below)
		decodeAndExecuteSub();
	} else if(command == 0x1A) {
		// GetID - Command 1Ah --> INT3(stat) --> INT2/5 (stat,flags,type,atip,"SCEx")
		GetID();
	} else {
		INT3();
		
		if(command == 0x6) {
			interrupts.push(1);
			responses.push(_stats);
		} else if(command == 0x9) {
			interrupts.push(2);
			responses.push(_stats);
		}
		
		// ReadTOC - Command 1Eh --> INT3(stat) --> INT2(stat)
		// ReadN - Command 06h --> INT3(stat) --> INT1(stat) --> datablock
		// Pause - Command 09h --> INT3(stat) --> INT2(stat)
		
		printf("");
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

void CDROM::GetStat() {
	// INT3 - First response: Acknowledge the command
	interrupts.push(3);
	
	/**
	 * 7  Play          Playing CD-DA         ;\only ONE of these bits can be set
	 * 6  Seek          Seeking               ; at a time (ie. Read/Play won't get
	 * 5  Read          Reading data sectors  ;/set until after Seek completion)
	 * 4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
	 * 3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
	 * 2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
	 * 1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
	 * 0  Error         Invalid Command/parameters (followed by Error Byte)
	 */
	
	// Lid is closed
	_stats = (_stats & (~0x18));
	_stats |= 0x2;
	
	// Turn motor ON ig?
	//_stats |= 1 << 1;
	
	responses.push(_stats);
}

void CDROM::SetLoc() {
	// Setloc - Command 02h,amm,ass,asect --> INT3(stat)
	
	/*
	 * Sets the seek target - but without yet starting the seek operation.
	 * 
	 * amm:    minute number on entire disk (00h and up)
	 * ass:    second number on entire disk (00h to 59h)
	 * asect:  sector number on entire disk (00h to 74h)
	 */
	
	uint8_t amm = parameters.front(); parameters.pop();
	uint8_t ass = parameters.front(); parameters.pop();
	uint8_t asect = parameters.front(); parameters.pop();
	
	// Convert BCD to binary
	uint8_t minutes = ((amm >> 4) * 10) + (amm & 0x0F);
	uint8_t seconds = ((ass >> 4) * 10) + (ass & 0x0F);
	uint8_t sectors = ((asect >> 4) * 10) + (asect & 0x0F);
	
	// TODO; Store the seek target (MSF format)
	uint32_t loc = sectors + (seconds * 75) + (minutes * 60 * 70);
	
	/**
	 * E = Error 80h appears on some commands (02h..09h, 0Bh..0Dh, 10h..16h, 1Ah, 1Bh?, and 1Dh)
	 * when the disk is missing, or when the drive unit is disconnected from the mainboard.
	 */
	
	if(!diskPresent) {
		interrupts.push(5);
		responses.push(0x11);
		responses.push(0x80);
		
		return;
	}
	
	INT3();
}

void CDROM::SeekL() {
	// TODO; Handle this
	
	// SeekL - Command 15h --> INT3(stat) --> INT2(stat)
	
	// TODO; This command will stop any current or pending ReadN or ReadS.
	// TODO; fter the seek, the disk stays on the seeked location forever,
	// (namely: when seeking sector N, it does stay at around N-8..N-0 in single speed mode,
	// or at around N-5..N+2 in double speed mode).
	
	// TODO; Seek to Setloc's location in data mode
	
	INT3();
	
	// TODO; This gets pushed once seek is completed
	responses.push(_stats);
	interrupts.push(2); 
}

void CDROM::GetID() {
	// TODO; Handle this
	
	// GetID - Command 1Ah --> INT3(stat) --> INT2/5 (stat,flags,type,atip,"SCEx")
	
	/*
	* Drive Status           1st Response   2nd Response
    * Door Open              INT5(11h,80h)  N/A
    * Spin-up                INT5(01h,80h)  N/A
    * Detect busy            INT5(03h,80h)  N/A
    * No Disk                INT3(stat)     INT5(08h,40h, 00h,00h, 00h,00h,00h,00h)
    * Audio Disk             INT3(stat)     INT5(0Ah,90h, 00h,00h, 00h,00h,00h,00h)
    * Unlicensed:Mode1       INT3(stat)     INT5(0Ah,80h, 00h,00h, 00h,00h,00h,00h)
    * Unlicensed:Mode2       INT3(stat)     INT5(0Ah,80h, 20h,00h, 00h,00h,00h,00h)
    * Unlicensed:Mode2+Audio INT3(stat)     INT5(0Ah,90h, 20h,00h, 00h,00h,00h,00h)
    * Debug/Yaroze:Mode2     INT3(stat)     INT2(02h,00h, 20h,00h, 20h,20h,20h,20h)
    * Licensed:Mode2         INT3(stat)     INT2(02h,00h, 20h,00h, 53h,43h,45h,4xh)
    * Modchip:Audio/Mode1    INT3(stat)     INT2(02h,00h, 00h,00h, 53h,43h,45h,4xh)
	*/
	
	// INT3 - First response: Acknowledge the command
	INT3();
	
	uint8_t flags = 0;
	uint8_t type = 0;
	uint8_t atip = 0; // Default 0x00 for most discs
	std::array<uint8_t, 4> scex = {0, 0, 0, 0};
	
	// TODO;
	bool isLicensedDisc = true;
	
	// TODO; INT2 if it's an audio disk
	// TODO; Cause error(stats) if no disk is present
	
	if (diskPresent) {
		if (isLicensedDisc) {
			// Licensed Mode2 disc
			flags = 0x00; // Not denied
			type = 0x20;  // Mode2
			
			// "SCEE" (Europe/PAL)
			scex = {'S', 'C', 'E', 'E'};
		} else {
			// Unlicensed Mode1 disc
			flags = 0x80; // Denied
			type = 0x00;  // Mode1
		}
	} else {
		// No disc present
		interrupts.push(5);
		
		flags = 0x40; // Missing disc
		type = 0x00;  // Unknown type
	}
	
	// Push the response bytes
	// INT2/5 (stat,flags,type,atip,"SCEx")
	responses.push(_stats);
	
	responses.push(flags); // flags
	responses.push(type);  // disc type
	responses.push(atip);  // atip
	responses.push(scex[0]); // SCEx region string (4 bytes)
	responses.push(scex[1]);
	responses.push(scex[2]);
	responses.push(scex[3]);
}

void CDROM::INT3() {
	interrupts.push(3);
	responses.push(_stats);
}
