#include "Timer.h"

#include "../../Utils/Bitwise.h"

/**
 * Caution: Reading the Current Counter Value can be a little unstable (when using dotclk or hblank as clock source);
 * the GPU clock isn't in sync with the CPU clock, so the timer may get changed during the CPU read cycle.
 * As a workaround: repeat reading the timer until the received value is the same (or slightly bigger) than the previous value.
 */

Emulator::IO::Timer::Timer() {
	
}

void Emulator::IO::Timer::step(uint32_t cycles) {
	counter += cycles;
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
	
	sync = Utils::Bitwise::getBit(val, 0);
	
	// Bits 1-2
	syncMode = (val & 0x06) >> 1;
	
	resetType = Utils::Bitwise::getBit(val, 3);
	interruptTarget = Utils::Bitwise::getBit(val, 4);
	interruptWrap = Utils::Bitwise::getBit(val, 5);
	interruptRepeatMode = Utils::Bitwise::getBit(val, 6);
	interruptToggleMode = Utils::Bitwise::getBit(val, 7);
	
	// I don't really get wtf this shit is
	// Bits 8-9
	source = (val & 0x0300) >> 8;
	
	// Bits 10-12 are read-only.
	
	// TODO; Make sure this is after writing
	interrupt = true;
	
	counter = 0;
}

uint16_t Emulator::IO::Timer::getMode() {
	uint16_t mode = 0;
	
	mode |= (sync) << 0;
	
	// Bits 1-2
	mode |= (syncMode) << 1;
	
	mode |= (resetType) << 3;
	mode |= (interruptTarget) << 4;
	mode |= (interruptWrap) << 5;
	mode |= (interruptRepeatMode) << 6;
	mode |= (interruptToggleMode) << 7;
	
	// Bits 8-9
	mode |= (source) << 8;
	
	mode |= (!interrupt) << 10;
	mode |= (reachedTarget) << 11;
	mode |= (hasWrapped) << 12;
	
	// TODO; Make sure this is after reading
	reachedTarget = false;
	hasWrapped = false;
	
	return mode;
}
