#pragma once

#include <stdint.h>

/**
 * C++ being the biggest bitch in existence rn
 */
#include "D:/libs/glm/glm/vec3.hpp"
#include "D:/libs/glm/glm/mat3x3.hpp"

class Instruction;

class COP2 {
public:
	COP2();
	
	void setControl(uint32_t r, uint32_t val);
	uint32_t getControl(uint32_t r);
	
	void setData(uint32_t r, uint32_t val);
	uint32_t getData(uint32_t r);
	
	void setReg(uint32_t r, uint32_t val);
	//uint32_t getReg(uint32_t r);
	
private:
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
	
	// cop2r32-36 9xS16 RT11RT12,..,RT33 Rotation matrix     (3x3)
	glm::mat3x3 rotation;
	
	// cop2r37-39 3x 32 TRX,TRY,TRZ
	int32_t TRX = 0;
	int32_t TRY = 0;
	int32_t TRZ = 0;
	
	// cop2r40-44 9xS16 L11L12,..,L33    Light source matrix (3x3)
	glm::mat3x3 lightSource;
	
	// Background Color (BK)
	int32_t RBK = 0; // COPR-45
	int32_t GBK = 0; // COPR-46
	int32_t BBK = 0; // COPR-47
	
	// cop2r48-52 9xS16 LR1LR2,..,LB3    Light color matrix source (3x3)
	glm::mat3x3 lightColorSource;
	
	// Far Color (FC)
	int32_t RFC = 0; // COPR-53
	int32_t GFC = 0; // COPR-54
	int32_t BFC = 0; // COPR-55
	
	// Screen offset X
	int32_t OFX = 0; // COPR-56
	
	// Screen offset Y
	int32_t OFY = 0; // COPR-57
	
	// Projection plane distance
	uint16_t H = 0; // COPR-58
	
	// Depth queing paramter A (coeff)
	int16_t DQA = 0; // COPR-59
	
	// Depth queing paramter B (coeff)
	int32_t DQB = 0; // COPR-60
	
	// Average Z registers
	int16_t zsf3 = 0; // COPR-61
	int16_t zsf4 = 0; // COPR-62

	// cop2r63      U20 FLAG             Returns any calculation errors
	uint32_t flag = 0;
	
private:
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
	
	// cop2r0-1   3xS16 VXY0,VZ0              Vector 0 (X,Y,Z)
	glm::vec3 VXYZ0, VXYZ1, VXYZ2;
	
private:
	
	uint32_t gte[32] = { };
};
