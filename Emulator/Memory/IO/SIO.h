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
			
			void setCtrl(uint32_t val);
			
		private:
			uint8_t txData = 0;
			uint32_t rxData = 0;
			uint32_t stat = 0;
			uint16_t mode = 0;
			//uint16_t ctrl = 0;
			
			bool isRXFull = false;
			
			uint16_t baudtimerRate = 0;
			uint16_t buadFactor = 0;
			uint32_t budTimer = 0;

			/* SIO#_CTRL (R/W)
			* 0     TX Enable (TXEN)      (0=Disable, 1=Enable)
			* 1     DTR Output Level      (0=Off, 1=On)
			* 2     RX Enable (RXEN)      (SIO1: 0=Disable, 1=Enable)  ;Disable also clears RXFIFO
			* 							   (SIO0: 0=only receive when /CS low, 1=force receiving single byte)
			* 3     SIO1 TX Output Level  (0=Normal, 1=Inverted, during Inactivity & Stop bits)
			* 4     Acknowledge           (0=No change, 1=Reset SIO_STAT.Bits 3,4,5,9)      (W)
			* 5     SIO1 RTS Output Level (0=Off, 1=On)
			* 6     Reset                 (0=No change, 1=Reset most registers to zero) (W)
			* 7     SIO1 unknown?         (read/write-able when FACTOR non-zero) (otherwise always zero)
			* 8-9   RX Interrupt Mode     (0..3 = IRQ when RX FIFO contains 1,2,4,8 bytes)
			* 10    TX Interrupt Enable   (0=Disable, 1=Enable) ;when SIO_STAT.0-or-2 ;Ready
			* 11    RX Interrupt Enable   (0=Disable, 1=Enable) ;when N bytes in RX FIFO
			* 12    DSR Interrupt Enable  (0=Disable, 1=Enable) ;when SIO_STAT.7  ;DSR high or /ACK low
			* 13    SIO0 port select      (0=port 1, 1=port 2) (/CS pulled low when bit 1 set)
			* 14-15 Not used              (always zero)
			*/
			bool txEnabled = false;
			bool dtrOutput = false;
			bool rxEnabled = false;
			bool sio1TxOutputLevel = false;
			bool ack = false;
			bool sio1RtsOutputLevel = false;
			bool reset = false;
			bool sio1Unknown = false;
			uint8_t rxInterruptMode = 0;
			bool txInterruptEnabled = false;
			bool rxInterruptEnabled = false;
			bool dsrInterruptEnabled = false;
			bool sio0Selected = false;
			
			bool interrupt = false;
			bool dsrInputLevel = false;
			
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
