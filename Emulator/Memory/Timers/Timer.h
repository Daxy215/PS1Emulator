#pragma once
#include <stdint.h>

namespace Emulator {
	class Gpu;
}

namespace Emulator {
	namespace IO {
		enum class TimerType {
			DotClock,
			HBlank,
			SystemClock8,
		};
		
		enum class Clock {
			SystemClock,
			SystemClock8,
			DotClock,
			HBlank,
		};
		
		class Timer {
		public:
			Timer(TimerType type);
			
			void step(uint32_t cycles);
			void syncGpu(bool isInHBlank, bool isInVBlank, uint32_t dot);
			
			void handleInterrupt();
			
			void setMode(uint16_t val);
			uint16_t getMode();
			
		public:
			double _cycles = 0;
			double counter = 0;
			uint16_t target = 0;
			
			// 0 = Free Run, 1 = Sync via Bit1-2
			bool sync = false;
			
			/**
			 * For timer 0 (GPU dotclock)
				 * 0 = Pause counter during Hblank(s)
				 * 1 = Reset counter to 0000h at HBlank
				 * 2 = Reset counter to 0000h at HBlank and pause outside HBlank
				 * 3 = Pause until HBlank occurs once, then switch to Free Run (turn off sync)
				 
			 * For timer 1 (GPU hBlank)
				* Same as timer 0 but uses VBlank instead of HBlank
				
			 * For timer 2 (System clock / 8)
				* 0 or 3 = Stop counter at current value (forever, no h/v-blank start)
				* 1 or 2 = Free Run (Same as when Sync is disabled)
			 */
			uint8_t syncMode = 0;
			
			/**
			 * Reset after:
				 * 0 = Counter reaches FFFFh
				 * 1 = Counter reaches Target
			 */
			bool resetType = false;
			
			/**
			 * Causes an interrupt if,
			 * counter reaches target
			 */
			bool interruptTarget = false;
			
			/**
			 * Causes an interrupt if,
			 * counter reaches FFFFh
			 */
			bool interruptWrap = false;
			
			/**
			 * 0 = One-shot
			 * 1 = Repeatedly
			 */
			bool interruptMode = false;
			
			/**
			 * 0 = Short Bit 10 = 0 Pulse
			 * 1 = Toggle Bit 10 on/off
			 */
			bool interruptToggleMode = false;
			
			/**
			 *  For Counter 0: 0 or 2 = System Clock, 1 or 3 = Dotclock
			 *  For Counter 1: 0 or 2 = System Clock, 1 or 3 = HBlank
			 *  For Counter 2: 0 or 1 = System Clock, 2 or 3 = System Clock / 8
			 */
			uint8_t source = 0;
			
			/**
			 * 0 = Yes
			 * 1 = No
			 * (Set after Writing)
			 */
			bool interrupt = false;
			
			/**
			 * 0 = No
			 * 1 = Yes
			 * (Reset after Reading)
			 */
			bool reachedTarget = false;
			bool hasWrapped = false;
			
		private:
			bool isInHBlank = false;
			bool isInVBlank = false;
			uint32_t dot = 1;
			
			bool wasInHBlank = false;
			bool wasInVBlank = false;
			
			bool wasIRQ = false;
			
		private:
			TimerType _type;
			
		};
	}
}
