#pragma once
#include <cstdint>
#include <queue>

#include "Joypad.h"

namespace Emulator {
	namespace IO {
		class SIO {
		enum ConnectedDevice {
			None,
			Controller,
			MemoryCard
		};
		
		public:
			void step(uint32_t cycles);
			
			uint32_t load(uint32_t addr);
			void store(uint32_t addr, uint32_t val);
			
		private:
			uint8_t txData = 0;
			uint32_t rxData = 0;
			uint32_t stat = 0;
			uint16_t mode = 0;
			uint16_t ctrl = 0;
			
			bool isRXFull = false;
			bool irq = false;
			
			uint16_t baudtimerRate = 0;
			uint16_t buadFactor = 0;
			uint32_t budTimer = 0;
			
			bool interrupt = false;
			bool dsrInputLevel = false;
			bool dtrOutput = false;
			bool sio0Selected = false;
			
			bool txIdle = true;
			bool txReady = true;
			
		private:
			int16_t timer = -1;
			
		private:
			ConnectedDevice _connectedDevice = None;
			
		private:
			/*std::queue<uint8_t> tx;
			std::queue<uint8_t> rx;*/
			
		private:
			// TODO; Temp
			Joypad _joypad = {};
		};
	}
}
