#pragma once

#include <stdio.h>

/**
 * C++ being the biggest bitch in existence rn
 */
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"

class COP2 {
public:
	union GTEInstruction {
		struct {
			uint32_t cmd                    : 6;
			uint32_t                        : 4; // Always zero
			uint32_t lm                     : 1;
			uint32_t                        : 2; // Always zero
			uint32_t mvmvaTranslationVector : 2; // (0=TR, 1=BK, 2=FC/Bugged, 3=None)
			uint32_t mvmvaMultiplyVector    : 2; // (0=V0, 1=V1, 2=V2, 3=IR/long)
			uint32_t mvmvaMultiplyMatrix    : 2; // (0=Rotation. 1=Light, 2=Color, 3=Reserved)
			uint32_t sf                     : 1;
			uint32_t                        : 5; // Ignored by hardware?
			uint32_t imm25                  : 7; // Must be 0100101b for "COP2 imm25" instructions
		};
		
		uint32_t reg;
		
		GTEInstruction() = default;	
		GTEInstruction(uint32_t reg) : reg(reg) {} 
	};
	
	union Flag {
		/**
		 * 31   Error Flag (Bit30..23, and 18..13 ORed together) (Read only)
		 * 30   MAC1 Result larger than 43 bits and positive
		 * 29   MAC2 Result larger than 43 bits and positive
		 * 28   MAC3 Result larger than 43 bits and positive
		 * 27   MAC1 Result larger than 43 bits and negative
		 * 26   MAC2 Result larger than 43 bits and negative
		 * 25   MAC3 Result larger than 43 bits and negative
		 * 24   IR1 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
		 * 23   IR2 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
		 * 22   IR3 saturated to +0000h..+7FFFh (lm=1) or to -8000h..+7FFFh (lm=0)
		 * 21   Color-FIFO-R saturated to +00h..+FFh
		 * 20   Color-FIFO-G saturated to +00h..+FFh
		 * 19   Color-FIFO-B saturated to +00h..+FFh
		 * 18   SZ3 or OTZ saturated to +0000h..+FFFFh
		 * 17   Divide overflow. RTPS/RTPT division result saturated to max=1FFFFh
		 * 16   MAC0 Result larger than 31 bits and positive
		 * 15   MAC0 Result larger than 31 bits and negative
		 * 14   SX2 saturated to -0400h..+03FFh
		 * 13   SY2 saturated to -0400h..+03FFh
		 * 12   IR0 saturated to +0000h..+1000h
		 * 0-11 Not used (always zero) (Read only)
		 */
		struct {
			uint32_t : 12; // Not used (always zero) (read only)
			uint32_t irq0Saturated        : 1;
			uint32_t sy2Saturated         : 1;
			uint32_t sx2Saturated         : 1;
			uint32_t mac0OverflowNegative : 1;
			uint32_t mac0OverflowPositive : 1;
			uint32_t divideOverflow       : 1;
			uint32_t sz3OtzSaturated      : 1;
			uint32_t colorBSaturated      : 1;
			uint32_t colorGSaturated      : 1;
			uint32_t colorRSaturated      : 1;
			uint32_t ir3Saturated         : 1;
			uint32_t ir2Saturated         : 1;
			uint32_t ir1Saturated         : 1;
			uint32_t mac3OverflowNegative : 1;
			uint32_t mac2OverflowNegative : 1;
			uint32_t mac1OverflowNegative : 1;
			uint32_t mac3OverflowPositive : 1;
			uint32_t mac2OverflowPositive : 1;
			uint32_t mac1OverflowPositive : 1;
			uint32_t errorFlag            : 1; // (Read only)
		};
		
		uint32_t reg;
		
		Flag() = default;	
		Flag(uint32_t reg) : reg(reg) {} 
	};
	
public:
	COP2();
	
	void decode(GTEInstruction instruction);
	
public:
	void setControl(uint32_t r, uint32_t val);
	uint32_t getControl(uint32_t r);
	
	void setData(uint32_t r, uint32_t val);
	uint32_t getData(uint32_t r);
	
	void setReg(uint32_t r, uint32_t val);
	//uint32_t getReg(uint32_t r);
	
private:
	void rtps();
	
	void rtpt();
	
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
	Flag flag = 0;
	
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
	
	// cop2r6     4xU8  RGBC                  Color/code value
	glm::vec4 RGBC;
	
	// cop2r7     1xU16 OTZ                   Average Z value (for Ordering Table)
	uint16_t OTZ = 0;
	
	// cop2r8     1xS16 IR0                   16bit Accumulator (Interpolate)
	int16_t IR0 = 0;
	
	// cop2r9-11  3xS16 IR1,IR2,IR3           16bit Accumulator (Vector)
	int16_t IR1 = 0;
	int16_t IR2 = 0;
	int16_t IR3 = 0;
	
	// cop2r12-15 6xS16 SXY0,SXY1,SXY2,SXYP   Screen XY-coordinate FIFO  (3 stages)
	glm::vec2 SXY0;
	glm::vec2 SXY1;
	glm::vec2 SXY2;
	glm::vec2 SXY3;
	
	// cop2r16-19 4xU16 SZ0,SZ1,SZ2,SZ3       Screen Z-coordinate FIFO   (4 stages)
	uint16_t SZ0 = 0;
	uint16_t SZ1 = 0;
	uint16_t SZ2 = 0;
	uint16_t SZ3 = 0;

	// cop2r24    1xS32 MAC0                  32bit Maths Accumulators (Value)
	uint32_t MAC0 = 0;

	// cop2r25-27 3xS32 MAC1,MAC2,MAC3        32bit Maths Accumulators (Vector)
	int32_t MAC1 = 0;
	int32_t MAC2 = 0;
	int32_t MAC3 = 0;
	
	// cop2r30-31 2xS32 LZCS,LZCR             Count Leading-Zeroes/Ones (sign bits)
	int32_t LZCS = 0;
	int32_t LZCR = 0;
	
private:
	uint32_t sf = 0, lm = 0;
	
private:
	
	uint32_t gte[32] = { };
};
