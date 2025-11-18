#pragma once
#include <cstdio>
#include <map>
#include <stdint.h>
#include <GLFW/glfw3.h>

#include "Peripherals/DigitalController.h"
#include "Peripherals/MemoryCard.h"

class GLFWwindow;

namespace Emulator {
	namespace IO {
		class SIO {
			enum ConnectedDevice {
				None,
				Controller,
				MemoryCard
			};
			
			union STAT {
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
				struct {
					uint32_t TX_FIFO_NOT_FULL       : 1; // (1=Ready for new byte)  (depends on CTS) (TX requires CTS)
					uint32_t RX_FIFO_NOT_EMPTY      : 1; // (0=Empty, 1=Data available)
					uint32_t TX_IDLE                : 1; // (1=Idle/Finished)       (depends on TXEN and on CTS)
					uint32_t RX_PARITY_ERROR		: 1; // (0=No, 1=Error; Wrong Parity, when enabled) (sticky)
					uint32_t SIO1_RX_FIFO_OVERRUN   : 1; // (0=No, 1=Error; received more than 8 bytes) (sticky)
					uint32_t SIO1_RX_BAD_STOP_BIT   : 1; // (0=No, 1=Error; Bad Stop Bit) (when RXEN)   (sticky)
					uint32_t SIO1_RX_INPUT_LEVEL    : 1; // (0=Normal, 1=Inverted) ;only AFTER receiving Stop Bit
					uint32_t DSR_INPUT_LEVEL        : 1; // (0=Off, 1=On) (remote DTR) ;DSR not required to be on
					uint32_t SIO1_CTS_INPUT_LEVEL   : 1; // (0=Off, 1=On) (remote RTS) ;CTS required for TX
					uint32_t INTERRUPT_REQUEST      : 1; // (0=None, 1=IRQ) (See SIO_CTRL.Bit4,10-12)   (sticky)
					uint32_t UNKNOWN                : 1; // ALWAYS ZERO
					uint32_t BAUDRATE_TIMER         : 20; // (15-21 bit timer, decrementing at 33MHz)
				};
				
				uint32_t reg = 0;
				
				explicit STAT(const uint32_t reg) : reg(reg) {}
			} __attribute__((packed));
			
			public:
				void step(uint32_t cycles);
				
				uint32_t load(uint32_t addr);
				void store(uint32_t addr, uint32_t val);
				
				void setCtrl(uint32_t val);
				
				static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
					if (action == GLFW_PRESS || action == GLFW_RELEASE) {
						bool isPressed = (action == GLFW_PRESS);
						
						// TODO; Handle 2nd controller input
						auto& controller = _controllers[0];
						
						switch (key) {
							case GLFW_KEY_ENTER:       controller.input.Start    = isPressed; break;
							case GLFW_KEY_BACKSPACE:   controller.input.Select   = isPressed; break;
							case GLFW_KEY_UP:          controller.input.Up       = isPressed; break;
							case GLFW_KEY_RIGHT:       controller.input.Right    = isPressed; break;
							case GLFW_KEY_DOWN:        controller.input.Down     = isPressed; break;
							case GLFW_KEY_LEFT:        controller.input.Left     = isPressed; break;
							case GLFW_KEY_Z:           controller.input.Cross    = isPressed; break;
							case GLFW_KEY_X:           controller.input.Circle   = isPressed; break;
							case GLFW_KEY_A:           controller.input.Square   = isPressed; break;
							case GLFW_KEY_S:           controller.input.Triangle = isPressed; break;
							case GLFW_KEY_Q:           controller.input.L1       = isPressed; break;
							case GLFW_KEY_E:           controller.input.R1       = isPressed; break;
							case GLFW_KEY_1:           controller.input.L2       = isPressed; break;
							case GLFW_KEY_2:           controller.input.R2       = isPressed; break;
							case GLFW_KEY_LEFT_SHIFT:  controller.input.L3       = isPressed; break;
							case GLFW_KEY_RIGHT_SHIFT: controller.input.R3       = isPressed; break;
							default: break;
						}
					}
				}
				
			private:
				uint8_t txData = 0;
				uint32_t rxData = 0;
				
				STAT stat = STAT(0);
				uint16_t mode = 0;
				//uint16_t ctrl = 0;
				
				uint16_t baudTimerRate = 0;
				uint16_t baudFactor = 0;
				uint32_t baudTimer = 0;
			
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
				
			private:
				int16_t timer = -1;
				
			private:
				ConnectedDevice _connectedDevice = None;
				
			private:
				/*std::queue<uint8_t> tx;
				std::queue<uint8_t> rx;*/
				
			private:
				// TODO; Temp
				static DigitalController _controllers[2];
				
				::MemoryCard _memoryCard = {};
		};
	}
}
