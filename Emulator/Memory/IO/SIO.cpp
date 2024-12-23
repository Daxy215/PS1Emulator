#include "SIO.h"

#include <cassert>
#include <iostream>

#include "../IRQ.h"
#include "../../Utils/Bitwise.h"

#include "GLFW/glfw3.h"

DigitalController Emulator::IO::SIO::_controler = {};

void Emulator::IO::SIO::step(uint32_t cycles) {
	if(timer > 0) {
		timer -= 1;
		
		if(timer == 0) {
			dsrInputLevel = false;
			interrupt = true;
			
			IRQ::trigger(IRQ::PadMemCard);
		}
	}
}

uint32_t Emulator::IO::SIO::load(uint32_t addr) {
	if(addr == 0x1F801040) {
		/**
		 * SIO#_RX_DATA (READ)
		 * 
		 * 0-7   Received Data      (1st RX FIFO entry) (oldest entry)
		 * 8-15  Preview            (2nd RX FIFO entry)
		 * 16-23 Preview            (3rd RX FIFO entry)
		 * 24-31 Preview            (4th RX FIFO entry) (5th..8th cannot be previewed)
		 */
		
		/**
		 * A data byte can be read when SIO_STAT.1=1.
		 * Some emulators behave incorrectly when this register,
		 * is read using a 16/32-bit memory access,
		 * so it should only be accessed as an 8-bit register.
		 */
		//assert(isRXFull);
		
		if(!isRXFull)
			return 0xFF;
		
		uint32_t data = 0;
		
		isRXFull = false;
		
		return rxData;
		
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
		
		stat |= budTimer << 11;
		stat |= (interrupt) << 9;
		stat |= (dsrInputLevel) << 7;
		
		// Bit 3: RX Parity Error (0=No, 1=Error; Wrong Parity, when enabled)
		// TODO;
		stat |= (0) << 3;
		stat |= (txIdle) << 2;
		stat |= (isRXFull) << 1;
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
		uint16_t ctrl = 0;
		
		ctrl |= (txEnabled) << 0;
		ctrl |= (dtrOutput) << 1;
		ctrl |= (rxEnabled) << 2;
		ctrl |= (sio1TxOutputLevel) << 3;
		//ctrl |= (ack) << 4; // Write-only
		ctrl |= (sio1RtsOutputLevel) << 5;
		//ctrl |= (reset) << 6; // Write-only
		ctrl |= (sio1Unknown) << 7;
		ctrl |= (rxInterruptMode) << 8;
		ctrl |= (txInterruptEnabled) << 10;
		ctrl |= (rxInterruptEnabled) << 11;
		ctrl |= (dsrInterruptEnabled) << 12;
		ctrl |= (sio0Selected) << 13;
		
		return ctrl;
	} else if(addr == 0x1F80105E) {
		/**
		 * SIO#_BAUD (R/W)
		 */
		
		return baudtimerRate;
	}
	
	return 0xFF;
}

void Emulator::IO::SIO::store(uint32_t addr, uint32_t val) {
	if(addr == 0x1F801040) {
		/**
		 * SIO#_TX_DATA (WRITE-ONLY)
		 *
		 * 0-7 - Data to be sent
		 * 8-32 - Not used
		 */
		
		auto f = (val & 0x7F) >> 21;
		txData = static_cast<uint8_t>(val);
		rxData = 0xFF;
		isRXFull = true;
		
		txReady = true;
		txIdle = false;
		
		// TODO; Start transfer;
		// If TXEN was set and is currently cleaned,
		// then it should still start a transfer
		// This is used in Wipeout 2097
		
		// If nothing is currently connected
		if(_connectedDevice == None) {
			if(val == 0x01) {
				// Controller
				_connectedDevice = Controller;
			} else if(val == 0x81) {
				// Memory card
				std::cerr << "MEMORY CARD\n";
				_connectedDevice = MemoryCard;
			} else {
				printf("");
			}
		}
		
		// Bit 1 of CTRL
		if(dtrOutput) {
			txIdle = true;
			
			// TODO; Handle 2nd port..
			/*if(sio0Selected) {
				rxData = 0xFF;
				dsrInputLevel = false;
				
				return;
			}*/
			
			if(_connectedDevice == Controller) {
				rxData = _controler.load(txData);
				dsrInputLevel = _controler._interrupt;
				
				if(dsrInputLevel) {
					timer = 5;
				}
			} else if(_connectedDevice == MemoryCard) {
				// TODO; Transfer data
				
				//if(dsrInputLevel) {
					//timer = 3;
				//}
			} else {
				dsrInputLevel = false;
			}
			
			if(!dsrInputLevel) {
				_connectedDevice = None;	
			}
		} else {
			_connectedDevice = None;
			
			// TODO; reset controller & memory card
			_controler.reset();
			
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
		
		buadFactor = mode & 0x3;
	} else if(addr == 0x1F80104A) {
		setCtrl(val);
	} else if(addr == 0x1F80104E) {
		/**
		 * SIO#_BAUD (R/W)
		 */
		
		baudtimerRate = static_cast<uint16_t>(val);
		budTimer = (baudtimerRate * buadFactor) & ~0x1;
	} else {
		printf("");
	}
}

void Emulator::IO::SIO::setCtrl(uint32_t val) {
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
	
	// It's seems to be stuck in a loop,
	// and fetching ctrl? So imma break it up,
	// and see if I can hopefully solve the issue..
	// or at least find it?
	uint16_t ctrl = static_cast<uint16_t>(val);
	
	txEnabled = Utils::Bitwise::getBit(ctrl, 0);
	dtrOutput = Utils::Bitwise::getBit(ctrl, 1);
	rxEnabled = Utils::Bitwise::getBit(ctrl, 2);
	sio1TxOutputLevel = Utils::Bitwise::getBit(ctrl, 3);
	ack = Utils::Bitwise::getBit(ctrl, 4);
	sio1RtsOutputLevel = Utils::Bitwise::getBit(ctrl, 5);
	reset = Utils::Bitwise::getBit(ctrl, 6);
	sio1Unknown = Utils::Bitwise::getBit(ctrl, 7);
	rxInterruptMode = Utils::Bitwise::getBit(ctrl, 8) & 0x3;
	txInterruptEnabled = Utils::Bitwise::getBit(ctrl, 10);
	rxInterruptEnabled = Utils::Bitwise::getBit(ctrl, 11);
	dsrInterruptEnabled = Utils::Bitwise::getBit(ctrl, 12);
	sio0Selected = Utils::Bitwise::getBit(ctrl, 13);
	
	if (ack) {
		interrupt = false;
		ack = false;
	}
	
	// Rest
	if (reset) {
		isRXFull = false;
		
		rxData = 0xFF;
		txData = 0xFF;
		
		txReady = true;
		txIdle = true;
		
		baudtimerRate = 0;
		
		_connectedDevice = None;
		_controler.reset();
		
		mode = 0;
		
		reset = false;
	}
	
	if (!dtrOutput) {
		_connectedDevice = None;
		_controler.reset();
	}
}

// TODO; Move this to a better location
void Emulator::IO::SIO::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		bool isPressed = (action == GLFW_PRESS);
		
		switch (key) {
			case GLFW_KEY_ENTER:       _controler._input.Start    = isPressed; break;
			case GLFW_KEY_BACKSPACE:   _controler._input.Select   = isPressed; break;
			case GLFW_KEY_UP:          _controler._input.Up       = isPressed; break;
			case GLFW_KEY_RIGHT:       _controler._input.Right    = isPressed; break;
			case GLFW_KEY_DOWN:        _controler._input.Down     = isPressed; break;
			case GLFW_KEY_LEFT:        _controler._input.Left     = isPressed; break;
			case GLFW_KEY_Z:           _controler._input.Cross    = isPressed; break;
			case GLFW_KEY_X:           _controler._input.Circle   = isPressed; break;
			case GLFW_KEY_A:           _controler._input.Square   = isPressed; break;
			case GLFW_KEY_S:           _controler._input.Triangle = isPressed; break;
			case GLFW_KEY_Q:           _controler._input.L1       = isPressed; break;
			case GLFW_KEY_E:           _controler._input.R1       = isPressed; break;
			case GLFW_KEY_1:           _controler._input.L2       = isPressed; break;
			case GLFW_KEY_2:           _controler._input.R2       = isPressed; break;
			case GLFW_KEY_LEFT_SHIFT:  _controler._input.L3       = isPressed; break;
			case GLFW_KEY_RIGHT_SHIFT: _controler._input.R3       = isPressed; break;
			default: break;
		}
	}
}
