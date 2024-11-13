#include "COP2.h"


/**
 * cop2r32-36 9xS16 RT11RT12,..,RT33 Rotation matrix     (3x3)        ;cnt0-4
 * cop2r37-39 3x 32 TRX,TRY,TRZ      Translation vector  (X,Y,Z)      ;cnt5-7
 * cop2r40-44 9xS16 L11L12,..,L33    Light source matrix (3x3)        ;cnt8-12
 * cop2r45-47 3x 32 RBK,GBK,BBK      Background color    (R,G,B)      ;cnt13-15
 * cop2r48-52 9xS16 LR1LR2,..,LB3    Light color matrix source (3x3)  ;cnt16-20
 * cop2r53-55 3x 32 RFC,GFC,BFC      Far color           (R,G,B)      ;cnt21-23
 * cop2r56-57 2x 32 OFX,OFY          Screen offset       (X,Y)        ;cnt24-25
 * cop2r58 BuggyU16 H                Projection plane distance.       ;cnt26
 * cop2r59      S16 DQA              Depth queing parameter A (coeff) ;cnt27
 * cop2r60       32 DQB              Depth queing parameter B (offset);cnt28
 * cop2r61-62 2xS16 ZSF3,ZSF4        Average Z scale factors          ;cnt29-30
 * cop2r63      U20 FLAG             Returns any calculation errors   ;cnt31
 */
void COP2::setControl(uint32_t r, uint32_t val) {
	// The register has an offset of 32
	r += 32;
	
	switch (r) {
		case 45: {
			RBK = static_cast<int32_t>(val);
			break;
		}
		case 46: {
			GBK = static_cast<int32_t>(val);
			break;
		}
		case 47: {
			BBK = static_cast<int32_t>(val);
			break;
		}
		case 53: {
			RFC = static_cast<int32_t>(val);
			break;
		}
		case 54: {
			GFC = static_cast<int32_t>(val);
			break;
		}
		case 55: {
			BFC = static_cast<int32_t>(val);
			break;
		}
		case 56: {
			OFX = static_cast<int32_t>(val);
			break;
		}
		case 57: {
			OFY = static_cast<int32_t>(val);
			break;
		}
		case 58: {
			H = static_cast<uint16_t>(val);
			break;
		}
		case 59: {
			DQA = static_cast<int16_t>(val);
			break;
		}
		case 60: {
			DQB = static_cast<int32_t>(val);
			break;
		}
		case 61: {
			zsf3 = static_cast<int16_t>(val);
			break;
		}
		
		case 62: {
			zsf4 = static_cast<int16_t>(val);
			break;
		}
		default: {
			//printf("");
			
			break;
		}
	}
}
 
uint32_t COP2::getControl(uint32_t r) {
	//printf("");
	return 0;
}

void COP2::setData(uint32_t r, uint32_t val) {
	//printf("");
}

uint32_t COP2::getData(uint32_t r) {
	//printf("");
	return 0;
}

void COP2::setReg(uint32_t r, uint32_t val) {
	//if(r >= 32)
		//throw std::runtime_error("???");
	
	gte[r] = val;
}

/*uint32_t COP2::getReg(uint32_t r) {
	if(r >= 32)
		throw std::runtime_error("???");
	
	return gte[r];
}*/
