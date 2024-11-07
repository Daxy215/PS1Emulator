#pragma once
#include <cstdint>

/**
 * This is a static class,
 * Making it to make my life a tiny bit easier
 */
namespace Emulator {
	namespace Timers {
		class Scheduler {
		public:
			static void tick(uint32_t ticks) {
				Scheduler::ticks += ticks;
			}
			
			static uint32_t getTicks() { return Scheduler::ticks; }
			
		private:
			inline static uint32_t ticks = 0;
		};
	}
}
