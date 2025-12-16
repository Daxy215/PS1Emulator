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
Emulator::IO::Timer::Timer(TimerType type) : type(type) {
	
}

void Emulator::IO::Timer::step(uint32_t cpuCycles) {
	_cycles += cpuCycles;
	
	// Synchronization
	if (mode.sync) {
		switch (type) {
			case TimerType::DotClock:
				switch (mode.syncMode) {
					case 0: // Pause during HBlank
						if (isInHBlank) return;
						break;
						
					case 1: // Reset at HBlank
						if (isInHBlank) {
							counter = 0;
							_cycles = 0;
							return;
						}
						
						break;
						
					case 2: // Reset at HBlank, pause outside
						if (isInHBlank) {
							counter = 0;
							_cycles = 0;
						} else {
							return;
						}
						
						break;
						
					case 3: // Pause until next HBlank edge, then free run
						if (!wasInHBlank && isInHBlank) {
							mode.sync = 0; // disable sync
						} else {
							return;
						}
						
						break;
				}
				
				break;
				
			case TimerType::HBlank:
				switch (mode.syncMode) {
					case 0: // Pause during VBlank
						if (isInVBlank) return;
						break;
						
					case 1: // Reset at VBlank
						if (isInVBlank) {
							counter = 0;
							_cycles = 0;
							return;
						}
						
						break;
						
					case 2: // Reset at VBlank, pause outside
						if (isInVBlank) {
							counter = 0;
							_cycles = 0;
						} else {
							return;
						}
						
						break;

					case 3: // Pause until VBlank edge, then free-run
						if (!wasInVBlank && isInVBlank) {
							mode.sync = 0;
						} else {
							return;
						}
						
						break;
				}
				
				break;
				
			case TimerType::SystemClock8:
				// syncMode 0 and 3 both pause timer
				if (mode.syncMode == 0 || mode.syncMode == 3) {
					_cycles = 0;
					return;
				}
				
				break;
		}
	}
	
	// Clock source
	switch (type) {
		case TimerType::DotClock:
			if (mode.source & 1) {
				// Dot clock = 1 tick per 6 CPU cycles
				finishTick(_cycles / 6);
				_cycles %= 6;
			} else {
				// System clock
				finishTick(_cycles);
				_cycles = 0;
			}
			
			break;
			
		case TimerType::HBlank:
			if (mode.source & 1) {
				// Count rising edge only
				if (!wasInHBlank && isInHBlank)
					finishTick(1);
				
				_cycles = 0;
			} else {
				// System clock
				finishTick(_cycles);
				_cycles = 0;
			}
			
			break;
			
		case TimerType::SystemClock8:
			if (mode.source >= 2) {
				// system clock / 8
				finishTick(_cycles / 8);
				_cycles %= 8;
			} else {
				// normal system clock
				finishTick(_cycles);
				_cycles = 0;
			}
			
			break;
	}
}

void Emulator::IO::Timer::tick() {
	counter++;
	
	if (counter == target) {
		mode.reachedTarget = true;
		
		if (mode.resetType) {
			counter = 0;
			holdAtZero = 2;
		}
		
		if (mode.interruptTarget)
			requestIRQ();
	}
	
	if (counter == 0x10000) {
		mode.hasWrapped = true;
		counter = 0;
		
		if (!mode.resetType)
			holdAtZero = 1;
		
		if (mode.interruptWrap)
			requestIRQ();
	}
}

void Emulator::IO::Timer::finishTick(uint32_t ticks) {
	uint32_t old = counter;
	uint32_t newValue = counter + ticks;
	
	if (!ignoreTargetUntilWrap && old < target && newValue >= target) {
		mode.reachedTarget = true;
		
		if (mode.resetType)
			newValue = 0;
		
		if (mode.interruptTarget)
			requestIRQ();
	}
	
	counter = newValue;
	
	// Overflow
	if (counter >= 0x10000) {
		mode.hasWrapped = true;
		counter &= 0xFFFF;
		
		ignoreTargetUntilWrap = false;
		
		if (mode.resetType == 0)
			counter = 0;
		
		if (mode.interruptWrap)
			requestIRQ();
	}
	
	counter &= 0xFFFF;
}

void Emulator::IO::Timer::requestIRQ() {
	if (mode.interruptMode == 0) {
		// One-shot: only fire if not already fired
		if (!mode.interrupt) return;
		mode.interrupt = false;
	} else {
		// Toggle mode or pulse mode
		if (mode.interruptToggleMode)
			mode.interrupt = !mode.interrupt;
		else
			mode.interrupt = false;
	}
	
	switch (type) {
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
}

void Emulator::IO::Timer::syncGpu(bool isInHBlank, bool isInVBlank, uint32_t dot) {
	this->wasInHBlank = this->isInHBlank;
	this->wasInVBlank = this->isInVBlank;
	
	this->isInHBlank = isInHBlank;
	this->isInVBlank = isInVBlank;
	
	this->dot = dot;
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
