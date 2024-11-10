#include "SIO.h"

#include <iostream>

#include "../IRQ.h"
#include "../../Utils/Bitwise.h"

void Emulator::IO::SIO::step(uint32_t cycles) {
	if(timer > 0) {
		timer--;
	} else if(timer == 0) {
		timer = -1;
		dtrOutput = false;
		interrupt = true;
		
		IRQ::trigger(IRQ::Controller);
	}
}

uint32_t Emulator::IO::SIO::load(uint32_t addr) {
	/*printf("SIO LOAD %x\n", addr);
	std::cerr << "";*/
	
	if(addr == 0x1F801040) {
		/**
		 * SIO#_RX_DATA (READ)
		 * 
		 * 0-7   Received Data      (1st RX FIFO entry) (oldest entry)
		 * 8-15  Preview            (2nd RX FIFO entry)
		 * 16-23 Preview            (3rd RX FIFO entry)
		 * 24-31 Preview            (4th RX FIFO entry) (5th..8th cannot be previewed)
		 */
		
		uint32_t data = 0;
		
		isRXFull = false;
		
		return rxData;
		
		/*for(int i = 0; i < 4; i++) {
			if(!rx.empty()) {
				// Read it in reverse,
				// as this is queue
				data |= (rx.back()) << ((3 - i) * 8);
				rx.pop();
			}
		}*/
		
		// TODO; Check wtf does this shit mean:
		
		/**
		 * A data byte can be read when SIO_STAT.1=1.
		 * Some emulators behave incorrectly when this register,
		 * is read using a 16/32-bit memory access,
		 * so it should only be accessed as an 8-bit register.
		 */
		
		return data;
 	} else if(addr == 0x1F801044) {
		/**
		 * SIO#_STAT (R)
		 * 
		 * 0     TX FIFO Not Full       (1=Ready for new byte)  (depends on CTS) (TX requires CTS)
         * 1     RX FIFO Not Empty      (0=Empty, 1=Data available)
         * 2     TX Idle                (1=Idle/Finished)       (depends on TXEN and on CTS)
         * 3     RX Parity Error        (0=No, 1=Error; Wrong Parity, when enabled) (sticky)
         * 4     SIO1 RX FIFO Overrun   (0=No, 1=Error; received more than 8 bytes) (sticky)
         * 5     SIO1 RX Bad Stop Bit   (0=No, 1=Error; Bad Stop Bit) (when RXEN)   (sticky)
         * 6     SIO1 RX Input Level    (0=Normal, 1=Inverted) ;only AFTER receiving Stop Bit
         * 7     DSR Input Level        (0=Off, 1=On) (remote DTR) ;DSR not required to be on
         * 8     SIO1 CTS Input Level   (0=Off, 1=On) (remote RTS) ;CTS required for TX
         * 9     Interrupt Request      (0=None, 1=IRQ) (See SIO_CTRL.Bit4,10-12)   (sticky)
         * 10    Unknown                (always zero)
         * 11-31 Baudrate Timer         (15-21 bit timer, decrementing at 33MHz)
		 */
		
		// Reset the stat register
		stat = 0;
		
		// Bits 11-31: Baudrate Timer (15-21 bit timer, decrementing at 33MHz)
		stat |= baudtimerRate << 11;
		
		// Bit 10; Unknown (always zero)
		stat |= (0) << 10;
		
		// Bit 9: Interrupt request (0=None, 1=IRQ) (See SIO_CTRL.Bit4, 10-12)
		// TODO;
		stat |= (interrupt) << 9;
		
		// Bit 8: SIO1 CTS Input Level (0=Off, 1=On) (remote RTS); CTS required for TX
		// TODO; Always zero
		stat |= (0) << 8;
		
		// Bit 7: DSR Input Level (0=Off, 1=On) (remote DTR); Isn't required to be on
		stat |= (dsrInputLevel) << 7;
		
		// Bit 6: SIO1 RX Input Level (0=Normal, 1=Inverted)
		// TODO; Always zero
		stat |= (0) << 6;
		
		// Bit 5: SIO1 RX Bad Stop Bit (0=No, 1=Error; Bad Stop Bit)
		// TODO; Always zero
		stat |= (0) << 5;
		
		// Bit 4: SIO1 RX FIFO Overrun (0=No, 1=Error; received more than 8 bytes)
		// TODO; Always zero
		stat |= (0) << 4;
		
		// Bit 3: RX Parity Error (0=No, 1=Error; Wrong Parity, when enabled)
		// TODO;
		stat |= (0) << 3;
		
		// Bit: 2: TX Idle (1=Idle/Finished)
		// TODO;
		stat |= (txIdle) << 2;
		
		// Bit: 1: RX FIFO Not Empty      (0=Empty, 1=Data available)
		// TODO;
		stat |= (isRXFull) << 1;
		
		// Bit 0: TX FIFO Not Full (1=Read for a new byte) (depends on CTS) (TX requires CTS)
		// TODO;
		stat |= (txReady) << 0;
		
		return stat;
	} else if(addr == 0x1F801048) {
		return mode;	
	} else if(addr == 0x1F80104A) {
		/**
		 * SIO#_CTRL (R/W)
		 * 
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
		
		return ctrl;
		
		/*uint16_t data = 0;
		
		data |= (1) >> 0; // TODO;
		data |= (dtrOutput) >> 1;
		data |= (1) >> 2; // TODO; 
		data |= (0) >> 3; // TODO;
		data |= (0) >> 4; // TODO;
		data |= (0) >> 5; // TODO;
		data |= (0) >> 6;
		data |= (0) >> 7; // Always zero
		data |= (0) >> 8;
		data |= (0) >> 10;
		
		return data;*/
	} else if(addr == 0x1F80105E) {
		/**
		 * SIO#_BAUD (R/W)
		 */
		
		return baudtimerRate;
	}
	
	return 0xFF;
}

void Emulator::IO::SIO::store(uint32_t addr, uint32_t val) {
	/*printf("SIO %x - %x\n", addr, val);
	std::cerr << "";*/
	
	if(addr == 0x1F801040) {
		/**
		 * SIO#_TX_DATA (WRITE-ONLY)
		 *
		 * 0-7 - Data to be sent
		 * 8-32 - Not used
		 */
		
		//txData = (val & 0x7F) >> 21;
		txData = static_cast<uint8_t>(val);
		rxData = 0xFF;
		isRXFull = true;
		
		txReady = true;
		txIdle = false;
		
		// TODO; Start transfer;
		// If TXEN was set and is currently cleaned,
		// then it should still start a transfer
		// This is used in Wipeout 2097

		// Bit 1 of CTRL
		if(dtrOutput) {
			txIdle = true;
			
			if(sio0Selected) {
				rxData = 0xFF;
				dsrInputLevel = false;
				
				return;
			}
			
			// If nothing is currently connected
			if(_connectedDevice == None) {
				if(val == 0x01) {
					// Controller
					std::cerr << "CONTROLLER\n";
					_connectedDevice = Controller;
				} else if(val == 0x81) {
					// Memory card
					std::cerr << "MEMORY CARD\n";
					_connectedDevice = MemoryCard;
				}
			}
			
			if(_connectedDevice == Controller) {
				rxData = _joypad.load(txData);
				dtrOutput = _joypad._interrupt;
				
				if(dtrOutput) {
					timer = 5;
				}
			} else if(_connectedDevice == MemoryCard) {
				// TODO; Transfer data
			} else {
				dsrInputLevel = false;
			}
			
			if(!dsrInputLevel) {
				_connectedDevice = None;	
			}
		} else {
			_connectedDevice = None;
			
			// TODO; reset controller & memory card
			_joypad.reset();
			
			dsrInputLevel = false;
		}
	} else if(addr == 0x1F801048) {
		/**
		 * SIO#_MODE (R/W)
		 * 
		 * 0-1   Baudrate Reload Factor     (1=MUL1, 2=MUL16, 3=MUL64) (or 0=MUL1 on SIO0, STOP on SIO1)
		 * 2-3   Character Length           (0=5 bits, 1=6 bits, 2=7 bits, 3=8 bits)
		 * 4     Parity Enable              (0=No, 1=Enable)
		 * 5     Parity Type                (0=Even, 1=Odd) (seems to be vice-versa...?)
		 * 6-7   SIO1 stop bit length       (0=Reserved/1bit, 1=1bit, 2=1.5bits, 3=2bits)
		 * 8     SIO0 clock polarity (CPOL) (0=High when idle, 1=Low when idle)
		 * 9-15  Not used (always zero)
		 */
		
		mode = static_cast<uint16_t>(val);
	} else if(addr == 0x1F80104A) {
		/**
		 * SIO#_CTRL (R/W)
		 * 
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
		
		ctrl = static_cast<uint16_t>(val);
		
		// Rest
		if(Utils::Bitwise::getBit(ctrl, 6)) {
			// TODO; Reset registers
			isRXFull = false;
			
			rxData = 0xFF;
			txData = 0xFF;
			
			baudtimerRate = 0;
			
			_connectedDevice = None;
		} else if(Utils::Bitwise::getBit(ctrl, 4)) {
			// If this shouldn't reset the registers,
			// and should acknowledge IRQ
			interrupt = false;
		}
		
		dtrOutput = Utils::Bitwise::getBit(ctrl, 1);
		sio0Selected = Utils::Bitwise::getBit(ctrl, 13);
	} else if(addr == 0x1F80104E) {
		/**
		 * SIO#_BAUD (R/W)
		 */
		
		baudtimerRate = static_cast<uint16_t>(val);
	}
}
