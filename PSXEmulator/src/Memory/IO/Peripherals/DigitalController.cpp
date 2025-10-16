#include "DigitalController.h"

#include <iostream>

DigitalController::DigitalController() : _input(0) {
	
}

void DigitalController::step(uint32_t cycles) {
	
}

uint16_t DigitalController::load(uint32_t val) {
	/**
      *   Send Reply Comment
      * 01h  Hi-Z  Controller address
      * 42h  idlo  Receive ID bit0..7 (variable) and Send Read Command (ASCII "B")
      * TAP  idhi  Receive ID bit8..15 (usually/always 5Ah)
      * MOT  swlo  Receive Digital Switches bit0..7
      * MOT  swhi  Receive Digital Switches bit8..15
      * --- transfer stops here for digital pad (or analog pad in digital mode) ---
      * 00h  adc0  Receive Analog Input 0 (if any) (eg. analog joypad or mouse)
      * 00h  adc1  Receive Analog Input 1 (if any) (eg. analog joypad or mouse)
      * --- transfer stops here for analog mouse ----------------------------------
      * 00h  adc2  Receive Analog Input 2 (if any) (eg. analog joypad)
      * 00h  adc3  Receive Analog Input 3 (if any) (eg. analog joypad)
      * --- transfer stops here for analog pad (in analog mode) -------------------
      * --- transfer stops here for nonstandard devices (steering/twist/paddle) ---
      */
	
	// TODO; https://psx-spx.consoledev.net/controllersandmemorycards/#controller-and-memory-card-multitap-adaptor
	
	switch (_mode) {
		case Idle: {
			if(val == 0x01) {
				_mode = Connected;
				_interrupt = true;
				
				return 0xFF;
			} else {
				// Clear data buffer
				std::queue<uint8_t> empty;
				std::swap(data, empty);
				
				_interrupt = false;
				
				return 0xFF;
			}
			
			break;
		}
		
		case Transferring: {
			uint8_t d = data.front();
			data.pop();
			
			_interrupt = !data.empty();
			
			if(!_interrupt)
				_mode = Idle;
			
			return d;
		}
		
		case Connected: {
			if(val == 0x42) {
				_mode = Transferring;
				_interrupt = true;
				
				uint8_t b0 = TYPE & 0xFF;
				uint8_t b1 = (TYPE >> 8) & 0xFF;
				
				uint8_t b2 = (~_input._byte[0]) & 0xFF;
				uint8_t b3 = (~_input._byte[1]) & 0xFF;
				
				data.push(b0);
				data.push(b1);
				data.push(b2);
				data.push(b3);
				
				uint8_t d = data.front();
				data.pop();
				
				return d;
			} else {
				_mode = Idle;
				
				// Clear data buffer
				std::queue<uint8_t> empty;
				std::swap(data, empty);
				
				_interrupt = false;
				
				return 0xFF;
			}
		}
	}
}

void DigitalController::reset() {
	_mode = Idle;
	_interrupt = false;
}
