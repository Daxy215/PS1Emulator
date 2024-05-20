#pragma once
#include <cstdint>

#include "Instruction.h"
#include "interconnect.h"

class Instruction;

class CPU {
public:
    CPU(Interconnect* interconnect) : regs{}, interconnect(interconnect) {
        
    }

    void executeNextInstruction();
    void decodeAndExecute(Instruction& instruction);
    void decodeAndExecuteSubFunctions(Instruction& instruction);
    
    // Instructions
    // TODO; Please move those in a different class future me!

    // Shift Left Logical
    void opsll(Instruction instruction);
    
    // Load Upper Immediate(LUI)
    void oplui(Instruction& instruction);
    
    // ori $8, $8, 0x243f
    // Puts the result of the "bitwise" or of $8 and 0x243f back into $8
    void opori(Instruction& instruction);

    // Bitwise or
    void opor(Instruction& instruction);
    
    // Store Word
    void opsw(Instruction& instruction);

    // Jump
    void op_j(Instruction& instruction);
    
    // Add Immediate Unsigned
    void addiu(Instruction& instruction);
    
    // Coprocessors handling
    void opcop0(Instruction& instruction);

    void opmtc0(Instruction& instruction);
    
    // Register related functions
    RegisterIndex reg(uint32_t index) {
        return regs[index];
    }
    
    void set_reg(uint32_t index, RegisterIndex val) {
        regs[index] = val;
        
        // We need to always rest R0 to 0
        regs[0] = {0};
    }
    
    // Memory related functions
    uint32_t load32(uint32_t addr);
    void store32(uint32_t addr, uint32_t val);
    
    // Helper functions
    uint32_t wrappingAdd(uint32_t a, uint32_t b);

public:
    // PC initial value should be at the beginning of the BIOS.
    uint32_t pc =  0xbfc00000;
    
    // Cop0; register 12; Status Register
    uint32_t sr = 0;
    
    // General purpose registers
    // First entry must always contain a 0.
    RegisterIndex regs[32];
    
private:
    Instruction* nextInstruction = new Instruction(0x0); //NOP
    
    Interconnect* interconnect;
};
