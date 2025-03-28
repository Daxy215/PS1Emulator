﻿#include "CDROM.h"

#include <array>
#include <cassert>
#include <iostream>

/**
 * Error Notes: If the command has been rejected (INT5 sent as 1st response)
 * then the 2nd response isn't sent (eg. on wrong number of parameters,
 * or if disc missing). If the command fails at a later stage (INT5 as 2nd response),
 * then there are cases where another INT5 occurs as 3rd response
 * (eg. on SetSession=02h on non-multisession-disk).
 */
CDROM::CDROM() : _readSector(Sector::RAW_BUFFER), _sector(Sector::RAW_BUFFER) {
	
}

void CDROM::step(uint32_t cycles) {
	/*if(!interrupts.empty() && IF == 0) {
		IF |= interrupts.front();
		interrupts.pop();
	}*/
	
	if(!interrupts.empty()) {
		interrupts.front().delay -= cycles;
		
		if(interrupts.front().delay <= 0) {
			transmittingCommand = false;
			
			if((IE & 7) & (interrupts.front()._interrupt & 7)) {
				IRQ::trigger(IRQ::Interrupt::CDROM);
			}
		}
	}
	
	//triggerInterrupt();
	
	/**
	 * 1. Command busy flag set immediately.
	 * 2. Response FIFO is populated.
	 * 3. Command is being processed.
	 * 4. Command busy flag is unset and parameter fifo is cleared.
	 * 5. Shortly after (around 1000-6000 cycles later), CDROM IRQ is fired.
	 */
	if(busyFor > -1)
		busyFor -= cycles;
	
	if(busyFor < 0)
		transmittingCommand = false;
	
	const int sectorsPerSecond = mode.speed ? 150 : 75;
	// (44100 * 768) -> CPU clock speed
	const int cyclesPerSector = (44100 * 768) / sectorsPerSecond;
	
	this->cycles += cycles;
	
	for(int i = 0; i < this->cycles / cyclesPerSector; i++) {
		handleSector();
	}
	
	this->cycles %= cyclesPerSector;
}

void CDROM::handleSector() {
	if(!_stats.read && !_stats.play)
		return;
	
	const std::array<uint8_t, 12> sync = {{0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00}};
	
	Location pos = Location::fromLBA(readLocation);
	_readSector.set(_disk.read(pos));
	
	readLocation++;
	
	INT(1, 0);
	addResponse(_stats._reg);
	
	if (memcmp(_readSector.data(), sync.data(), sync.size()) != 0) {
		//assert(false);
		return;
	}
	
	// uint8_t minute = rawSector[12];
	// uint8_t second = rawSector[13];
	// uint8_t frame = rawSector[14];
	uint8_t mode = _readSector.loadAt(15);
	
	uint8_t file = _readSector.loadAt(16);
	uint8_t channel = _readSector.loadAt(17);
	//auto submode = static_cast<cd::Submode>(_readSector.loadAt(18));
	//auto codinginfo = static_cast<cd::Codinginfo>(_readSector.loadAt(19));
	
	assert(mode == 2);
	
	
}

template <>
uint32_t CDROM::load<uint32_t>(uint32_t addr) {
	/*printf("CDROM Load %x\n", addr);
	std::cerr << "";*/
	
	if(addr == 0) {
		uint32_t stat = 0;
		
		// 0-1 Index   Port 1F801801h-1F801803h index (0..3 = Index0..Index3)   (R/W)
		stat |= (_index) << 0;
		
		// 2 ADPBUSY XA-ADPCM fifo empty  (0=Empty) ;set when playing XA-ADPCM sound
		stat |= 0 << 2;
		
		// 3 PRMEMPT Parameter fifo empty (1=Empty) ;triggered before writing 1st byte
		stat |= (parameters.empty() ? 1 : 0) << 3;
		
		// 4 PRMWRDY Parameter fifo full  (0=Full)  ;triggered after writing 16 bytes	
		stat |= (parameters.size() >= 16 ? 0 : 1) << 4;
		
		// 5 RSLRRDY Response fifo empty  (0=Empty) ;triggered after reading LAST byte
		bool responseFifo = ((interrupts.empty() || interrupts.front().responses.empty()));
		stat |= (!responseFifo) << 5;
		
		// 6 DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
		stat |= (isBufferEmpty) << 6;
		
		//7 BUSYSTS Command/parameter transmission busy  (1=Busy)
		stat |= (transmittingCommand ? 1 : 0) << 7;
		
		return stat;
	} else if(addr == 1) {
		/*if(responses.size() <= 0) {
			printf("");
			return 0;
		}
		
		auto response = responses.front();
		responses.pop();
		
		return response;*/
		
		uint8_t response = 0;
		
		if(!interrupts.empty()) {
			auto& in = interrupts.front();
			
			if(!in.responses.empty()) {
				response = in.responses.front();
				in.responses.pop();
				
				if(in.responses.empty() && in.ack == true) {
					interrupts.pop();
				}
			}
		}
		
		return response;
	} else if(addr == 2) {
		return readByte();
	} else if(addr == 3) {
		switch (_index) {
		/*case 0: return IE | 0xE0;
		case 1: return IF | 0xE0;
		case 2: return IE; // Mirrored
		case 3: return IF; // Mirrored
		default:
			printf("");
			break;*/
			case 0:
			case 2:
				return IE;
			case 1:
			case 3: {
				uint8_t res = 0b11100000;
				
				if(!interrupts.empty() && interrupts.front().delay <= 0) {
					res |= interrupts.front()._interrupt & 7;
				}
				
				return res;
			}
		}
		
		return 0;
	}
	
	return 0;
}

template <>
void CDROM::store<uint32_t>(uint32_t addr, uint32_t val) {
	/*printf("CDROM write %x %x\n", addr, val);
	std::cerr << "";*/
	
	if(addr == 0) {
		// https://psx-spx.consoledev.net/cdromdrive/#1f801800h-indexstatus-register-bit0-1-rw-bit2-7-read-only
		// READ-ONLY
		_index = static_cast<uint8_t>(val & 0x3);
		
		return;
	} else if(addr == 1) {
		switch (_index) {
			case 0: {
				// Command register
				decodeAndExecute(val & 0xFF);
				
				break;
			}
		default:
			printf("");
			break;
		}
	} else if(addr == 2) {
		switch (_index) {
			case 0: {
				parameters.push(val & 0xFF);
				
				break;
			}
			
			case 1: {
				IE = val & 0xFF;
				
				break;
			}
			default:
				printf("");
				break;
		}
	} else if(addr == 3) {
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
					if(isEmpty()) {
						_sector.set(_readSector.read());
						isBufferEmpty = true;
					}
				} else {
					_sector.empty();
					 isBufferEmpty = false;
				}
				
				break;
			}
			
			case 1: {
				//IF &= ~(val & 0x1F);
				//IF = val & 0x1F;
				//triggerInterrupt();
				
				if (!interrupts.empty()) {
					interrupts.front().ack = true;
					
					if (interrupts.front().responses.empty()) {
						interrupts.pop();
					}
				}
				
				if((val & 0x40)) {
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

uint8_t CDROM::readByte() {
	if (_sector.isEmpty()) {
		return 0;
	}
	
	// 0 - 0x800 - just data
	// 1 - 0x924 - whole sector without sync
	int dataStart = 12;
	if (!mode.sectorSize) dataStart += 12;
	
	/**
	* The PSX hardware allows to read 800h-byte or 924h-byte sectors,
	* indexed as [000h..7FFh] or [000h..923h], when trying to read further bytes,
	* then the PSX will repeat the byte at index [800h-8] or [924h-4] as padding value.
	*/
	if (!mode.sectorSize && _sector._pointer >= 0x800) {
		_sector._pointer++;
		return _sector.loadAt(dataStart + 0x800 - 8);
	} else if (mode.sectorSize && _sector._pointer >= 0x924) {
		_sector._pointer++;
		return _sector.loadAt(dataStart + 0x924 - 4);
	}
	
	uint8_t data = _sector.loadAt(dataStart + _sector._pointer++);
	
	if(isEmpty()) {
		isBufferEmpty = false;
	}
	
	return data;
}

void CDROM::swapDisk(const std::string& path) {
	_disk.set(path);
	
	diskPresent = true;
}

void CDROM::decodeAndExecute(uint8_t command) {
	while(!interrupts.empty())
		interrupts.pop();
	
	if(command == 0x01) {
		//   01h Getstat      -               INT3(stat)
		GetStat();
	} else if(command == 0x02) {
		// Setloc - Command 02h,amm,ass,asect --> INT3(stat)
		SetLoc();
	} else if(command == 0x06) {
		// ReadN - Command 06h --> INT3(stat) --> INT1(stat) --> datablock
		ReadN();
	} else if(command == 0x08) {
		// 0x08Stop - Command 08h --> INT3(stat) --> INT2(stat)
		Stop();
	} else if(command == 0x09) {
		// Pause - Command 09h --> INT3(stat) --> INT2(stat)
		Pause();
	} else if(command == 0x0E) {
		// Setmode - Command 0Eh,mode --> INT3(stat)
		SetMode();
	} else if(command == 0x0A) {
		// Init - Command 0Ah --> INT3(stat) --> INT2(stat)
		Init();
	} else if(command == 0x15) {
	 	// SeekL - Command 15h --> INT3(stat) --> INT2(stat)
		SeekL();
	} else if(command == 0x19) { 
		//   19h Test         sub_function    depends on sub_function (see below)
		decodeAndExecuteSub();
	} else if(command == 0x1A) {
		// GetID - Command 1Ah --> INT3(stat) --> INT2/5 (stat,flags,type,atip,"SCEx")
		GetID();
	} else if(command == 0x1B) {
		// ReadS - Command 1Bh --> INT3(stat) --> INT1(stat) --> datablock
		ReadS();
	} else {
		if(command == 0x03) {
			// Too many args
			if (parameters.size() > 1) {
				INT(5);
				addResponse(0x01);
				addResponse(0x20);
				
				return;
			}
			
			auto toBinary = [](uint8_t b) -> uint8_t {
				int hi = (b >> 4) & 0xF;
				int lo = b & 0xF;
				
				return hi * 10 + lo; 
			};
			
			int trackNo = 0;
			if (parameters.size() == 1) {
				trackNo = toBinary(getParamater());
			}
			
			Location pos = Location::fromLBA(readLocation);
			
			// Start playing track n
			if (trackNo > 0) {
				int track = std::min(trackNo - 1, (int)_disk.tracks.size() - 1);
				pos = _disk.getTrackStart(track);
			} else {
				if (readLocation != 0) {
					pos = Location::fromLBA(readLocation);
					readLocation = 0;
				}
			}
			
			readLocation = pos.toLba();
			_stats.setMode(Stats::Mode::Playing);
			
			INT(3);
			addResponse(_stats._reg);
			
			return;
		} else if(command == 0x11) {
			INT(3, 1000);
			addResponse(0); // track
			addResponse(1); // index
			addResponse(0); // minute (track)
			addResponse(0); // second (track)
			addResponse(0); // sector (track)
			addResponse(0); // minute (disc)
			addResponse(0); // second (disc)
			addResponse(0); // sector (disc)
			
			// TODO;
			//assert(false);
			
			return;
		} else if(command == 0x13) {
			auto toBcd = [](uint8_t b) -> uint8_t {
				return ((b / 10) << 4) | (b % 10); 
			};
			
			INT3();
			addResponse(toBcd(0x01));
			addResponse(toBcd(_disk.tracks.size()));
			
			return;
		} else if(command == 0x14) {
			// GetTD - Command 14h,track --> INT3(stat,mm,ss) ;BCD
			auto toBinary = [](uint8_t b) -> uint8_t {
				int hi = (b >> 4) & 0xF;
				int lo = b & 0xF;
				
				return hi * 10 + lo; 
			};
			
			auto toBcd = [](uint8_t b) -> uint8_t {
				return ((b / 10) << 4) | (b % 10); 
			};
			
			auto track = toBinary(getParamater());
			
			if (track == 0) {  // end of last track
				auto diskSize = _disk.getSize();
				
				INT(3);
				addResponse(_stats._reg);
				addResponse(toBcd(diskSize.minutes));
				addResponse(toBcd(diskSize.seconds));
			} else if (track <= _disk.tracks.size()) {  // Start of n track
				auto start = _disk.getTrackStart(track - 1);
				
				INT(3);
				addResponse(_stats._reg);
				addResponse(toBcd(start.minutes));
				addResponse(toBcd(start.seconds));
			} else {  // error
				INT(5);
				addResponse(0x10);
				
				return;
			}
			
			return;
		}
		
		INT3();
		if(command != 0x0C && command != 0x0B) {
			// ReadTOC - Command 1Eh --> INT3(stat) --> INT2(stat)
			// ReadN - Command 06h --> INT3(stat) --> INT1(stat) --> datablock
			// Pause - Command 09h --> INT3(stat) --> INT2(stat)
			// Demute - Command 0Ch --> INT3(stat)
			// Mute - Command 0Bh --> INT3(stat)
			
			printf("");
		}
	}
	
	/**
	 * 1. Command busy flag set immediately.
     * 2. Response FIFO is populated.
     * 3. Command is being processed.
     * 4. Command busy flag is unset and parameter fifo is cleared.
     * 5. Shortly after (around 1000-6000 cycles later), CDROM IRQ is fired.
     * 
     * Not sure how to calculate the correct cycles count?
	 */
	busyFor = 1000;
	transmittingCommand = true;
	
	while(!parameters.empty())
		parameters.pop();
}

void CDROM::decodeAndExecuteSub() {
	if(parameters.empty()) {
		printf("");
		return;
	}
	
	uint8_t command = parameters.front();
	parameters.pop();
	
	if(command == 0x20) {
		//   20h      -   INT3(yy,mm,dd,ver) ;Get cdrom BIOS date/version (yy,mm,dd,ver)
		
		//interrupts.push(3);
		INT(3);
		
		// 95h,05h,16h,C1h  ;PSX (LATE-PU-8)          16 May 1995, version vC1 (a)
		addResponse(0x95);
		addResponse(0x05);
		addResponse(0x16);
		addResponse(0xC1);
	} else {
		printf("");
	}
}

bool CDROM::isEmpty() {
	if(_sector.isEmpty()) return true;
	
	if(!mode.sectorSize && _sector._pointer >= 0x800) return true;
	if(mode.sectorSize && _sector._pointer >= 0x924) return true;
	
	return false;
}

void CDROM::GetStat() {
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
	
	//_stats.shellOpen = 0;
	//_stats.motor = 1;
	
	INT(3);
	addResponse(_stats._reg);
	//responses.push(_stats._reg);
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
	
	uint8_t amm   = parameters.front(); parameters.pop();
	uint8_t ass   = parameters.front(); parameters.pop();
	uint8_t asect = parameters.front(); parameters.pop();
	
	auto toBinary = [](uint8_t bcd) -> uint8_t {
		return ((bcd >> 4) & 0xF) * 10 + (bcd & 0xF);
	};
	
	// Convert BCD to binary
	uint8_t minutes = toBinary(amm);
	uint8_t seconds = toBinary(ass);
	uint8_t sectors = toBinary(asect);
	
	seekLocation = sectors + (seconds * 75) + (minutes * 60 * 75);
	
	/**
	 * E = Error 80h appears on some commands (02h..09h, 0Bh..0Dh, 10h..16h, 1Ah, 1Bh?, and 1Dh)
	 * when the disk is missing, or when the drive unit is disconnected from the mainboard.
	 */
	if(!diskPresent) {
		INT(5);
		addResponse(0x11);
		addResponse(0x80);
		
		return;
	}
	
	INT(3, 5000);
	addResponse(_stats._reg);
}

void CDROM::ReadN() {
	// ReadN - Command 06h --> INT3(stat) --> INT1(stat) --> datablock
	
	if(!diskPresent) {
		INT(5);
		addResponse(0x11);
		addResponse(0x80);
		
		return;
	}
	
	readLocation = seekLocation;
	_stats.setMode(Stats::Mode::Reading);
	
	INT(3, 1000);
	addResponse(_stats._reg);
}

void CDROM::Stop() {
	// Stop - Command 08h --> INT3(stat) --> INT2(stat)
	
	// Stops motor with magnetic brakes
	// moves the drive head to the beginning of the first track.
	_stats.setMode(Stats::Mode::None);
	// TODO; Stop audio
	_stats.motor = 0;
	
	INT3();
	INT2();
}

void CDROM::Pause() {
	INT3();
	
	// the second response returns the new status (with bit5 cleared).
	_stats.setMode(Stats::Mode::None);
	// TODO; Pause audio?
	
	INT2();
}

void CDROM::SetMode() {
	// Setmode - Command 0Eh,mode --> INT3(stat)
	/**
	 * 7   Speed       (0=Normal speed, 1=Double speed)
     * 6   XA-ADPCM    (0=Off, 1=Send XA-ADPCM sectors to SPU Audio Input)
     * 5   Sector Size (0=800h=DataOnly, 1=924h=WholeSectorExceptSyncBytes)
     * 4   Ignore Bit  (0=Normal, 1=Ignore Sector Size and Setloc position)
     * 3   XA-Filter   (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
     * 2   Report      (0=Off, 1=Enable Report-Interrupts for Audio Play)
     * 1   AutoPause   (0=Off, 1=Auto Pause upon End of Track) ;for Audio Play
     * 0   CDDA        (0=Off, 1=Allow to Read CD-DA Sectors; ignore missing EDC)
	 */
	auto parm = getParamater();
	
	mode = Mode(parm);
	
	INT(3, 2000);
	addResponse(_stats._reg);
}

void CDROM::Init() {
	// Init - Command 0Ah --> INT3(stat) --> INT2(stat)
	INT(3, 0x13CE);
	addResponse(_stats._reg);
	
	_stats.setMode(Stats::Mode::None);
	
	mode._reg = 0;
	
	INT2();
}

void CDROM::SeekL() {
	// TODO; Handle this
	
	// SeekL - Command 15h --> INT3(stat) --> INT2(stat)
	
	// TODO; This command will stop any current or pending ReadN or ReadS.
	// TODO; fter the seek, the disk stays on the seeked location forever,
	// (namely: when seeking sector N, it does stay at around N-8..N-0 in single speed mode,
	// or at around N-5..N+2 in double speed mode).
	
	// TODO; Seek to Setloc's location in data mode
	
	readLocation = seekLocation;
	
	/**
	 * E = Error 80h appears on some commands (02h..09h, 0Bh..0Dh, 10h..16h, 1Ah, 1Bh?, and 1Dh)
	 * when the disk is missing, or when the drive unit is disconnected from the mainboard.
	 */
	if(!diskPresent) {
		INT(5);
		addResponse(0x11);
		addResponse(0x80);
		
		return;
	}
	
	INT(3, 5000);
	addResponse(_stats._reg);
	
	// TODO; This gets pushed once seek is completed
	INT(2, 500000);
	addResponse(_stats._reg);
	
	_stats.setMode(Stats::Mode::None);
}

void CDROM::GetID() {
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
	
	if (_stats.getShell()) {
		INT(5);
		addResponse(0x11);
		addResponse(0x80);
		
		return;
	}
	
	// INT3 - First response: Acknowledge the command
	INT3();
	
	if (diskPresent) {
		INT(2);
	} else {
		INT(5);
		// TODO;
	}
	
	// Licensed:Mode2         INT3(stat)     INT2(02h,00h, 20h,00h, 53h,43h,45h,4xh)
	addResponse(0x02); // Stat
	addResponse(0x00); // Flags
	addResponse(0x20); // Type
	addResponse(0x00); // Atip (Always zero)
	
	addResponse('S');
	addResponse('C');
	addResponse('E');
	addResponse('E');
}

void CDROM::ReadS() {
	readLocation = seekLocation;
	
	// TODO; Clear audio?
	_stats.setMode(Stats::Mode::Reading);
	
	INT(3, 500);
	addResponse(_stats._reg);
}

void CDROM::INT2() {
	INT(2);
	addResponse(_stats._reg);
}

void CDROM::INT3() {
	INT(3);
	addResponse(_stats._reg);
}

uint8_t CDROM::getParamater() {
	auto parm = parameters.front();
	parameters.pop();
	
	return parm;
}
