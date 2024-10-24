#include "CDROM.h"

void CDROM::decodeAndExecute(uint8_t command) {
	if(command == 0x19) {
		//   19h Test         sub_function    depends on sub_function (see below)
		decodeAndExecuteSub();
	}
}

void CDROM::decodeAndExecuteSub() {
	uint8_t command = paramaters.front();
	paramaters.pop();
	
	if(command == 0x20) {
		//   20h      -   INT3(yy,mm,dd,ver) ;Get cdrom BIOS date/version (yy,mm,dd,ver)
		
		// TODO;
		responses.push(0x94);
		responses.push(0x09);
		responses.push(0x19);
		responses.push(0xC0);
	}
}
