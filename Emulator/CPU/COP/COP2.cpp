#include "COP2.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>

// https://hitmen.c02.at/files/docs/psx/gte.txt

COP2::COP2() {
	
}

void COP2::decode(GTEInstruction instruction) {
	// reset FLAG register
	flag.reg = 0;
	
	sf = instruction.sf;
	lm = instruction.lm;
	
	/**
	 * Opc  Name   Clk Expl.
     * 01h  RTPS   15  Perspective Transformation single
     * 06h  NCLIP  8   Normal clipping
     * 0Ch  OP(sf) 6   Cross product of 2 vectors
     * 10h  DPCS   8   Depth Cueing single
     * 11h  INTPL  8   Interpolation of a vector and far color vector
     * 12h  MVMVA  8   Multiply vector by matrix and add vector (see below)
     * 13h  NCDS   19  Normal color depth cue single vector
     * 14h  CDP    13  Color Depth Que
     * 16h  NCDT   44  Normal color depth cue triple vectors
     * 1Bh  NCCS   17  Normal Color Color single vector
     * 1Ch  CC     11  Color Color
     * 1Eh  NCS    14  Normal color single
     * 20h  NCT    30  Normal color triple
     * 28h  SQR(sf)5   Square of vector IR
     * 29h  DCPL   8   Depth Cue Color light
     * 2Ah  DPCT   17  Depth Cueing triple (should be fake=08h, but isn't)
     * 2Dh  AVSZ3  5   Average of three Z values
     * 2Eh  AVSZ4  6   Average of four Z values
     * 30h  RTPT   23  Perspective Transformation triple
     * 3Dh  GPF(sf)5   General purpose interpolation
     * 3Eh  GPL(sf)5   General purpose interpolation with base
     * 3Fh  NCCT   39  Normal Color Color triple vector
	 */
	
	switch(instruction.cmd) {
	case 0x01: {
		//   01h  RTPS   15  Perspective Transformation single
		rtps();
		
		break;
	}
	case 0x06: {
		// 06h  NCLIP  8   Normal clipping
		
		break;
	}
	case 0x13: {
		// 13h  NCDS   19  Normal color depth cue single vector
		
		break;
	}
	case 0x30: {
		// 30h  RTPT   23  Perspective Transformation triple
		rtpt();
		
		break;
	}
	default:
		printf("Unhandled GTE instruction %x\n", instruction.cmd);
		std::cerr << "";
		
		break;
	}
}

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
	// Just to make it easier on myself
	r += 32;
	
	// TODO; This is ugly af
	if(r >= 32 && r <= 36) {
		int index = r - 32;
		
		int row = index / 3;
		int col = index % 3;
		
		rotation[row][col] = val;
	} else if (r == 37) {
		TRX = static_cast<int32_t>(val);
	} else if (r == 38) {
		TRY = static_cast<int32_t>(val);
	} else if (r == 39) {
		TRZ = static_cast<int32_t>(val);
	} else if(r >= 40 && r <= 44) {
		int index = r - 40;
		
		int row = index / 3;
		int col = index % 3;
		
		lightSource[row][col] = val;
	} else if (r == 45) {
		RBK = static_cast<int32_t>(val);
	} else if (r == 46) {
		GBK = static_cast<int32_t>(val);
	} else if (r == 47) {
		BBK = static_cast<int32_t>(val);
	} else if(r >= 48 && r <= 52) {
		int index = r - 48;
		
		int row = index / 3;
		int col = index % 3;
		
		lightColorSource[row][col] = val;
	} else if (r == 53) {
		RFC = static_cast<int32_t>(val);
	} else if (r == 54) {
		GFC = static_cast<int32_t>(val);
	} else if (r == 55) {
		BFC = static_cast<int32_t>(val);
	} else if (r == 56) {
		OFX = static_cast<int32_t>(val);
	} else if (r == 57) {
		OFY = static_cast<int32_t>(val);
	} else if (r == 58) {
		H = static_cast<uint16_t>(val);
	} else if (r == 59) {
		DQA = static_cast<int16_t>(val);
	} else if (r == 60) {
		DQB = static_cast<int32_t>(val);
	} else if (r == 61) {
		zsf3 = static_cast<int16_t>(val);
	} else if (r == 62) {
		zsf4 = static_cast<int16_t>(val);
	} else {
		printf("");
	}
}

uint32_t COP2::getControl(uint32_t r) {
	// The register has an offset of 32
	// Just to make it easier on myself
	r += 32;
	
	if (r >= 32 && r <= 36) {
		int index = r - 32;
		int row = index / 3;
		int col = index % 3;
		return rotation[row][col];
	} else if (r == 37) {
		return TRX;
	} else if (r == 38) {
		return TRY;
	} else if (r == 39) {
		return TRZ;
	} else if (r >= 40 && r <= 44) {
		int index = r - 40;
		int row = index / 3;
		int col = index % 3;
		return lightSource[row][col];
	} else if (r == 45) {
		return RBK;
	} else if (r == 46) {
		return GBK;
	} else if (r == 47) {
		return BBK;
	} else if (r >= 48 && r <= 52) {
		int index = r - 48;
		int row = index / 3;
		int col = index % 3;
		return lightColorSource[row][col];
	} else if (r == 53) {
		return RFC;
	} else if (r == 54) {
		return GFC;
	} else if (r == 55) {
		return BFC;
	} else if (r == 56) {
		return OFX;
	} else if (r == 57) {
		return OFY;
	} else if (r == 58) {
		return H;
	} else if (r == 59) {
		return DQA;
	} else if (r == 60) {
		return DQB;
	} else if (r == 61) {
		return zsf3;
	} else if (r == 62) {
		return zsf4;
	} else if(r == 63) {
		return flag.reg;
	} else {
		return 0;
	}
	
	printf("");
	return 0;
}

void COP2::setData(uint32_t r, uint32_t val) {
	/*
	 * cop2r0-1   3xS16 VXY0,VZ0              Vector 0 (X,Y,Z)
	 * cop2r2-3   3xS16 VXY1,VZ1              Vector 1 (X,Y,Z)
	 * cop2r4-5   3xS16 VXY2,VZ2              Vector 2 (X,Y,Z)
	 * cop2r6     4xU8  RGBC                  Color/code value
	 * cop2r7     1xU16 OTZ                   Average Z value (for Ordering Table)
	 * cop2r8     1xS16 IR0                   16bit Accumulator (Interpolate)
	 * cop2r9-11  3xS16 IR1,IR2,IR3           16bit Accumulator (Vector)
	 * cop2r12-15 6xS16 SXY0,SXY1,SXY2,SXYP   Screen XY-coordinate FIFO  (3 stages)
	 * cop2r16-19 4xU16 SZ0,SZ1,SZ2,SZ3       Screen Z-coordinate FIFO   (4 stages)
	 * cop2r20-22 12xU8 RGB0,RGB1,RGB2        Color CRGB-code/color FIFO (3 stages)
	 * cop2r23    4xU8  (RES1)                Prohibited
	 * cop2r24    1xS32 MAC0                  32bit Maths Accumulators (Value)
	 * cop2r25-27 3xS32 MAC1,MAC2,MAC3        32bit Maths Accumulators (Vector)
	 * cop2r28-29 1xU15 IRGB,ORGB             Convert RGB Color (48bit vs 15bit)
	 * cop2r30-31 2xS32 LZCS,LZCR             Count Leading-Zeroes/Ones (sign bits)
	 */
	
	auto setXY = [this](glm::vec3& vec, uint32_t val) {
		vec.x = static_cast<uint16_t>(val);
		vec.y = static_cast<uint16_t>(val >> 16);
	};
	
	// Vector 1
	if(r == 0) {
		setXY(VXYZ0, val);
	} else if(r == 1) {
		VXYZ0.z = static_cast<uint16_t>(val);
	}
	
	// Vector 2
	else if(r == 2) {
		setXY(VXYZ1, val);
	} else if(r == 3) {
		VXYZ1.z = static_cast<uint16_t>(val);
	}
	
	// Vector 3
	else if(r == 4) {
		setXY(VXYZ2, val);
	} else if(r == 5) {
		VXYZ2.z = static_cast<uint16_t>(val);
	}
	
	else if(r == 6) {
		RGBC = glm::vec4((val >> 24) & 0xFF, (val >> 16) & 0xFF,
						 (val >> 8 ) & 0xFF, (val >> 0 ) & 0xFF);
	} else if(r == 8) {
		IR0 = static_cast<uint16_t>(val);
	}
	
	else {
 		printf("");
	}
}

uint32_t COP2::getData(uint32_t r) {
	if(r == 7) {
		// TODO; cop2r7     1xU16 OTZ                   Average Z value (for Ordering Table)
	} else if(r == 8) {
		return IR0;
	} else if(r >= 12 && r <= 15) {
		// TODO; cop2r12-15 6xS16 SXY0,SXY1,SXY2,SXYP   Screen XY-coordinate FIFO  (3 stages)
	} else if(r >= 20 && r <= 22) {
		// TODO; cop2r20-22 12xU8 RGB0,RGB1,RGB2        Color CRGB-code/color FIFO (3 stages)
	} else if(r == 24) {
		// TODO; cop2r24    1xS32 MAC0                  32bit Maths Accumulators (Value)
	} else {
		printf("");
	}
	
	return 0;
}

void COP2::setReg(uint32_t r, uint32_t val) {
	if(r >= 32)
		throw std::runtime_error("???");
	
	gte[r] = val;
}

void COP2::rtps() {
	
}

void COP2::rtpt() {
	// RTPT is the same as RTPS, but repeats for V1 and V2. The "sf" bit should usually be set.
	
}

/*uint32_t COP2::getReg(uint32_t r) {
	if(r >= 32)
		throw std::runtime_error("???");
	
	return gte[r];
}*/
