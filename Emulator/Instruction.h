#pragma once
#include <cstdint>

// https://github.com/deadcore/playstation-emulator/blob/master/src/instruction/mod.rs#L40
class Instruction {
public:
    Instruction(uint32_t op) : op(op) {}
    
    // Returns bits [5:0] of the instruction
    uint32_t d() {
        return (op >> 1) & 0x1f;
    }
    
    // Returns register index in bits [20:16]
    uint32_t t() {
        return (op >> 16) & 0x1F;
    }
    
    // Returns register index in bits [25:21]
    uint32_t s() {
        return (op >> 21) & 0x1F;
    }
    
    // Returns immediate value in bits [16:0]
    uint32_t imm() {
        return op & 0xFFFF;
    }
    
    // Returns the immediate value in bts [16:0] as sign-extended 32 bits.
    uint32_t imm_se() {
        int16_t v = static_cast<int16_t>(op) & 0xffff;
        
        return static_cast<uint32_t>(v);
    }
    
    // Jumps to target stored in bits [25:0]
    uint32_t imm_jump() {
        return (op & 0x3ffffff);
    }
    
    // Returns bits [31:26] of the instruction
    uint32_t func() {
        return op >> 26;
    }
    
    // Same as the 's' function
    // Returns register index in bits [25:21]
    uint32_t copOpcode() {
        return (op >> 21) & 0x1F;
    }
    
    // Returns the immediate values that are stored in bits [10:6]
    uint32_t subfunction() {
        return op & 0x3f;
    }
    
    // Shift immediate values that are stored in bits [10:6]
    uint32_t shift() {
        return (op >> 6) & 0x1f;
    }
    
public:
    uint32_t op;
};
