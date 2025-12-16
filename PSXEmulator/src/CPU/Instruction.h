#pragma once

#include <stdint.h>

// https://github.com/deadcore/playstation-emulator/blob/master/src/instruction/mod.rs#L40
class Instruction {
    public:
        Instruction(uint32_t op) : op(op) {
            func    = (op >> 26) & 0x3F;
            rs      = (op >> 21) & 0x1F;
            rt      = (op >> 16) & 0x1F;
            rd      = (op >> 11) & 0x1F;
            shamt   = (op >> 6)  & 0x1F;
            subfunc =  op        & 0x3F;
            
            imm     =  op & 0xFFFF;
            imm_se  = (int16_t)(op & 0xFFFF);
            jump    =  op & 0x03FFFFFF;
        }
        
        /*// Returns bits [15:11] of the instruction
        uint32_t d() {
            return rd;
        }
        
        // Returns register index in bits [20:16]
        uint32_t t() {
            return rt;
        }
        
        // Returns register index in bits [25:21]
        uint32_t s() {
            return rs;
        }*/
        
        // Returns immediate value in bits [16:0]
        /*uint32_t imm() {
            return op & 0xFFFF;
        }*/
        
        // Returns the immediate value in bts [16:0] as sign-extended 32 bits.
        /*int16_t imm_se() {
            //return static_cast<uint32_t>(static_cast<int16_t>(op & 0xFFFF));
            return (static_cast<int16_t>(op & 0xFFFF));
        }*/
        
        // Jumps to target stored in bits [25:0]
        /*uint32_t imm_jump() {
            return jump;
        }*/
        
        // Returns bits [31:26] of the instruction
        /*uint32_t func() {
            return (op >> 26) & 0x3F;
        }
        */
        
        // Returns the immediate values that are stored in bits [5:0]
        /*uint32_t subfunction() {
            return subfunc;
        }
        
        // Same as the 's' function
        // Returns register index in bits [25:21]
        uint32_t copOpcode() {
            return rs;
        }
        
        // Shift immediate values that are stored in bits [10:6]
        uint32_t shift() {
            return shamt;
        }*/
        
    public:
        uint32_t op;
        
    public:
        uint32_t func;        // bits 31:26
        uint32_t rs;          // bits 25:21
        uint32_t rt;          // bits 20:16
        uint32_t rd;          // bits 15:11
        uint32_t shamt;       // bits 10:6
        uint32_t subfunc;     // bits 5:0
        
        uint32_t imm;         // zero-extended imm
        int16_t imm_se;       // sign-extended imm
        uint32_t jump;        // 26-bit jump target
};
