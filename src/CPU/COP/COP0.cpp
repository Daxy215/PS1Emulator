#include "COP0.h"

uint32_t COP0::cause = 0;

COP0::COP0() {
	
}

void COP0::reset() {
	bpc = 0;
	bda = 0;
	dcic = 0;
	bdam = 0;
	bpcm = 0;
	badVaddr = 0;
	sr = 0;
	cause = 0;
	epc = 0;
}
