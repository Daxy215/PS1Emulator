#include "Timer.h"

#include <GLFW/glfw3native.h>

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
	cycles -= holdAtZero;
	
	//if(paused)
	//	return;
	
	_cycles += cycles;
	
	//t = counter;
	
	switch (_type) {
	case TimerType::DotClock:
		if(mode.sync) {
			switch (mode.syncMode) {
				case 0: { if (isInHBlank) { _cycles -= cycles; return; } break; }
				case 1: { if (isInHBlank) { _cycles = 0; counter = 0; } break; }
				case 2: { if (isInHBlank) { _cycles = 0; counter = 0; } else { _cycles = 0; return; } break; }
				case 3: {
					if(!wasInHBlank && isInHBlank) {
						mode.sync = false;
					} else {
						_cycles = 0;
						
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
			/*counter += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);*/
			counter += static_cast<int>(_cycles);
			_cycles = 0;
		} else {
			// Use Dot Clock
			counter += _cycles / 6;//(_cycles * 11 / 7 / static_cast<double>(this->dot));
			_cycles %= 6;
		}
		
		break;
	case TimerType::HBlank:
		if(mode.sync) {
			switch (mode.syncMode) {
				case 0: { if (isInVBlank) { return; } break; }
				case 1: { if (isInVBlank) counter = 0; break; }
				case 2: { if (isInVBlank) counter = 0; else return; break; }
				case 3: {
					if(!wasInVBlank && isInVBlank) {
						mode.sync = false;
					}
					
					return;
				}
			}
		}
		
		if(mode.source == 0 || mode.source == 2) {
			// Use System Clock
			/*counter += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);*/
			counter += static_cast<int>(_cycles);
			_cycles = 0;
		} else {
			if(!wasInHBlank && isInHBlank) {
				counter++;
			}
			
			//counter += _cycles / 3413;
			//_cycles %= 3413;
			/*t += cycles / 768;
			_cycles %= 768;*/
		}
		
		break;
	case TimerType::SystemClock8:
		if(mode.sync && (mode.syncMode == 0 || mode.syncMode == 3)) {
			// Paused
			_cycles = 0;
			
			break;
		}
		
		if(mode.source == 0 || mode.source == 1) {
			// Use System Clock
			/*counter += static_cast<int>(_cycles * 1.5f);
			_cycles %= static_cast<int>(1.5f);*/
			counter += static_cast<int>(_cycles);
			_cycles = 0;
		} else {
			// Use System Clock / 8
			counter += (int)(_cycles / (8/* * 1.5f*/));
			_cycles %= static_cast<int>(8/* * 1.5f*/);
			//_cycles = Utils::modf(_cycles, 8);
		}
		
		break;
	}
	
	handleInterrupt();
	
	//counter = static_cast<uint16_t>(t);
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
	
	if(target == 10) {
		printf("");
	}
	
	if (!ignoreTargetUntilWrap && counter >= target) {
		mode.reachedTarget = true;
		
		if (mode.resetType) {
			counter = 0;
			holdAtZero = 2;
		}
		
		if(mode.interruptTarget) {
			shouldInterrupt = true;
		}
	}
	
	if(counter >= 0xFFFF) {
		mode.hasWrapped = true;
		
		if (ignoreTargetUntilWrap) {
			ignoreTargetUntilWrap = false;
		}
		
		if (!mode.resetType) {
			counter = 0;
			holdAtZero = 1;
		}
		
		if(mode.interruptWrap) {
			shouldInterrupt = true;
		}
	}
	
	counter &= 0xFFFF;
	
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
	
	mode._reg = val;
	
	// Reset values after writing
 	mode.interrupt = true;
	wasIRQ = false;
	
	counter = 0;
	holdAtZero = 2;
 }

uint16_t Emulator::IO::Timer::getMode() {
	auto f = mode._reg;
	
	mode.reachedTarget = false;
	mode.hasWrapped = false;
	
	return f;
}

void Emulator::IO::Timer::reset() {
	_cycles = 0;
	counter = 0;
	target = 0;
	mode = {};
	
	/*isInHBlank = false;
	isInVBlank = false;
	dot = 1;
	
	wasInHBlank = false;
	wasInVBlank = false;*//**/
	
	wasIRQ = false;
	
	// Keep type :}
	//_type = {};
}
