#pragma once
#include <stdint.h>

class Instruction;

class COP2 {
public:
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
	
	// Background Color (BK)
	int32_t RBK = 0; // COPR-45
	int32_t GBK = 0; // COPR-46
	int32_t BBK = 0; // COPR-47
	
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
	
	uint32_t gte[32] = { };
};
