#include "Timer.h"

#include "../IRQ.h"
#include "../../GPU/Gpu.h"

/**
 * Caution: Reading the Current Counter Value can be a little unstable (when using dotclk or hblank as clock source);
 * the GPU clock isn't in sync with the CPU clock, so the timer may get changed during the CPU read cycle.
 * As a workaround: repeat reading the timer until the received value is the same (or slightly bigger) than the previous value.
 */

/**
 * 1, 0, 1, 0, 0, 1, 0, 1, 0, 1,
 * 2, 2, 3, 2, 2, 3, 2, 2, 3, 2,
 */

Emulator::IO::Timer::Timer(TimerType type) : _type(type) {
	
}

void Emulator::IO::Timer::step(uint32_t cycles) {
	//if(paused)
	//	return;
	
	_cycles += cycles;
	
	t = counter;
	
	switch (_type) {
	case TimerType::DotClock:
		if(mode.sync) {
			switch (mode.syncMode) {
				case 0: { if (isInHBlank) { t = 0; return; } break; }
				case 1: { if (isInHBlank) t = 0; break; }
				case 2: { if (isInHBlank) t = 0; else { t = 0; return; } break; }
				case 3: {
					if(!wasInHBlank && isInHBlank) {
						mode.sync = false;
					} else {
						t = 0;
						
						return;
					}
					
					break;
				}
			default:
				printf("");
				break;
			}
		}
		
		if(mode.source == 0 || mode.source == 2) {
			// Use System Clock
			t += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);
		} else {
			// Use Dot Clock
			t += _cycles / 6;//(_cycles * 11 / 7 / static_cast<double>(this->dot));
			_cycles %= 6;
		}
		
		break;
	case TimerType::HBlank:
		if(mode.sync) {
			switch (mode.syncMode) {
				//case 0: { if (isInVBlank) { _cycles = 0; return; } break; }
				//case 1: { if (isInVBlank) t = 0; break; }
				case 2: { if (isInVBlank) t = 0; break; }
				case 3: {
					if(!wasInVBlank && isInVBlank) {
						mode.sync = false;
					} else {
						return;
					}
					
					break;
				}
			}
		}
		
		if(mode.source == 0 || mode.source == 2) {
			// Use System Clock
			t += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);
		} else {
			//if(!wasInHBlank && isInHBlank) counter++;
			t += _cycles / 3413;
			_cycles %= 3413;
		}
		
		break;
	case TimerType::SystemClock8:
		if(mode.sync && (mode.syncMode == 0 || mode.syncMode == 3)) {
			// Paused
			_cycles = 0;
			
			return;
		}
		
		auto source = mode.source & 1;
		
		if(source) {
			// Use System Clock
			t += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);
		} else {
			// Use System Clock / 8
			t += (int)(_cycles / (8 * 1.5f));
			_cycles %= static_cast<int>(8 * 1.5f);
			//_cycles = Utils::modf(_cycles, 8);
		}
		
		break;
	}
	
	handleInterrupt();
	
	counter = static_cast<uint16_t>(t);
}

void Emulator::IO::Timer::syncGpu(bool isInHBlank, bool isInVBlank, uint32_t dot) {
	this->wasInHBlank = this->isInHBlank;
	this->wasInVBlank = this->isInVBlank;
	
	this->isInHBlank = isInHBlank;
	this->isInVBlank = isInVBlank;
	
	this->dot = dot;
}

void Emulator::IO::Timer::handleInterrupt() {
	bool shouldInterrupt = false;
	
	if(t >= target) {
		mode.reachedTarget = true;
		
		if(mode.resetType) t = 0;
		
		if(mode.interruptTarget) {
			shouldInterrupt = true;
		}
	}
	
	if(t >= 0xFFFF) {
		mode.hasWrapped = true;
		
		if(!mode.resetType) t = 0;
		
		if(mode.interruptWrap) {
			shouldInterrupt = true;
		}
	}
	
	if(!shouldInterrupt)
		return;
	
	if(!mode.interruptToggleMode) {
		mode.interrupt = false;
	} else {
		mode.interrupt = !mode.interrupt;
	}
	
	//bool trigger = mode.interrupt == 0;
	
	if(!mode.interruptMode) {
		if(!wasIRQ/* && trigger*/) {
			wasIRQ = true;
		} else {
			return;
		}
	}
	
	// Trigger interrupt based on timer counter
	switch (_type) {
	case TimerType::DotClock:
		IRQ::trigger(IRQ::Timer0);
		break;
	case TimerType::HBlank:
		IRQ::trigger(IRQ::Timer1);
		break;
	case TimerType::SystemClock8:
		IRQ::trigger(IRQ::Timer2);
		break;
	}
	
	mode.interrupt = true;
}

void Emulator::IO::Timer::setMode(uint16_t val) {
	/**
	 * 0     Synchronization Enable (0=Free Run, 1=Synchronize via Bit1-2)
     * 1-2   Synchronization Mode   (0-3, see lists below)
     * 			Synchronization Modes for Counter 0:
     * 		 		0 = Pause counter during Hblank(s)
     * 		 		1 = Reset counter to 0000h at Hblank(s)
     * 		 		2 = Reset counter to 0000h at Hblank(s) and pause outside of Hblank
     * 		 		3 = Pause until Hblank occurs once, then switch to Free Run
     * 
     * 		 Synchronization Modes for Counter 1:
     * 			Same as above, but using Vblank instead of Hblank
     * 
     * 		 Synchronization Modes for Counter 2:
     * 		 	0 or 3 = Stop counter at current value (forever, no h/v-blank start)
     * 		 	1 or 2 = Free Run (same as when Synchronization Disabled)
     * 
     * 3     Reset counter to 0000h  (0=After Counter=FFFFh, 1=After Counter=Target)
     * 4     IRQ when Counter=Target (0=Disable, 1=Enable)
     * 5     IRQ when Counter=FFFFh  (0=Disable, 1=Enable)
     * 6     IRQ Once/Repeat Mode    (0=One-shot, 1=Repeatedly)
     * 7     IRQ Pulse/Toggle Mode   (0=Short Bit10=0 Pulse, 1=Toggle Bit10 on/off)
     * 
     * 8-9   Clock Source (0-3, see list below)
     * 		 	Counter 0:  0 or 2 = System Clock,  1 or 3 = Dotclock
     * 		 	Counter 1:  0 or 2 = System Clock,  1 or 3 = Hblank
     * 		 	Counter 2:  0 or 1 = System Clock,  2 or 3 = System Clock/8
     * 
     * 10    Interrupt Request       (0=Yes, 1=No) (Set after Writing)    (W=1) (R)
     * 11    Reached Target Value    (0=No, 1=Yes) (Reset after Reading)        (R)
     * 12    Reached FFFFh Value     (0=No, 1=Yes) (Reset after Reading)        (R)
	 */
	
	/*sync = Utils::Bitwise::getBit(val, 0);
	
	// Bits 1-2
	syncMode = (val >> 1) & 0x3;
	
	resetType = Utils::Bitwise::getBit(val, 3);
	interruptTarget = Utils::Bitwise::getBit(val, 4);
	interruptWrap = Utils::Bitwise::getBit(val, 5);
	interruptMode = Utils::Bitwise::getBit(val, 6);
	interruptToggleMode = Utils::Bitwise::getBit(val, 7);
	
	// Bits 8-9
	source = (val >> 8) & 0x3;*/
	
	mode._reg = val;
	
	// Values retested after a write
	mode.interrupt = true;
	wasIRQ = false;
	//paused = false;
	
	counter = 0;
}

uint16_t Emulator::IO::Timer::getMode() {
	mode.reachedTarget = false;
	mode.hasWrapped = false;
	
	return mode._reg;
	
	/*uint16_t mode = 0;
	
	mode |= (sync) << 0;
	
	// Bits 1-2
	mode |= (syncMode) << 1;
	
	mode |= (resetType) << 3;
	mode |= (interruptTarget) << 4;
	mode |= (interruptWrap) << 5;
	mode |= (interruptMode) << 6;
	mode |= (interruptToggleMode) << 7;
	
	// Bits 8-9
	mode |= (source) << 8;
	
	mode |= (!interrupt) << 10;
	mode |= (reachedTarget) << 11;
	mode |= (hasWrapped) << 12;
	
	
	
	return mode;*/
}
