#pragma once
#include <stdint.h>

struct RegisterIndex {
    RegisterIndex() : reg(0) {}
    RegisterIndex(uint32_t reg) : reg(reg) {}
    
    RegisterIndex operator|(const RegisterIndex& other) const {
        return {reg | other.reg};
    }
    
    operator uint32_t() const {
        return reg;
    }
    
    uint32_t reg;
    uint32_t lastWrite = 0;
};

// https://github.com/deadcore/playstation-emulator/blob/master/src/instruction/mod.rs#L40
class Instruction {
public:
    Instruction(uint32_t op) : op(op) {}
    
    // Returns bits [15:11] of the instruction
    RegisterIndex d() {
        return {(op >> 11) & 0x1F};
    }
    
    // Returns register index in bits [20:16]
    RegisterIndex t() {
        return {(op >> 16) & 0x1F};
    }
    
    // Returns register index in bits [25:21]
    RegisterIndex s() {
        return {(op >> 21) & 0x1F};
    }
    
    // Returns immediate value in bits [16:0]
    RegisterIndex imm() {
        return {op & 0xFFFF};
    }
    
    // Returns the immediate value in bts [16:0] as sign-extended 32 bits.
    uint32_t imm_se() {
        return static_cast<uint32_t>(static_cast<int16_t>(op & 0xFFFF));
    }
    
    // Jumps to target stored in bits [25:0]
    RegisterIndex imm_jump() {
        return {(op & 0x3FFFFFF)};
    }
    
    // Returns bits [31:26] of the instruction
    RegisterIndex func() {
        return {op >> 26};
    }
    
    // Returns the immediate values that are stored in bits [5:0]
    RegisterIndex subfunction() {
        return op & 0x3F;
    }
    
    // Same as the 's' function
    // Returns register index in bits [25:21]
    uint32_t copOpcode() {
        return (op >> 21) & 0x1F;
    }
    
    // Shift immediate values that are stored in bits [10:6]
    RegisterIndex shift() {
        return (op >> 6) & 0x1F;
    }
    
public:
    uint32_t op;
};
