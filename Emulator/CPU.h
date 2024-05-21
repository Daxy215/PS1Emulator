#pragma once
#include <cstdint>
#include <optional>

#include "Instruction.h"
#include "interconnect.h"

class Instruction;

// Used for the load register
struct Load {
    RegisterIndex regIndex;
    uint32_t value;
    
    Load(RegisterIndex reg, uint32_t val) : regIndex(reg), value(val) {}
};

class CPU {
public:
    CPU(Interconnect* interconnect) : outRegs{}, regs{}, interconnect(interconnect) {
        
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

    // Load word
    void oplw(Instruction instruction);
    
    // Jump
    void opj(Instruction& instruction);
    
    // Branch if not equal
    void opbne(Instruction& instruction);
    
    // Branch to immediate value 'offset'
    void branch(uint32_t offset);
    
    // Add Immediate Unsigned
    void addiu(Instruction& instruction);

    // Same as addiu but generates an exception if it overflows
    void addi(Instruction instruction);
    
    // Coprocessors handling
    void opcop0(Instruction& instruction);

    // Also Coprocessor handling but only for COP0
    void opmtc0(Instruction& instruction);
    
    // Register related functions
    RegisterIndex reg(uint32_t index) {
        return regs[index];
    }
    
    void set_reg(uint32_t index, RegisterIndex val) {
        outRegs[index] = val;
        
        // We need to always rest R0 to 0
        outRegs[0] = {0};
    }
    
    // Memory related functions
    uint32_t load32(uint32_t addr);
    void store32(uint32_t addr, uint32_t val);
    
    // Helper functions
    uint32_t wrappingAdd(uint32_t a, uint32_t b);
    uint32_t wrappingSub(uint32_t a, uint32_t b);

    template<typename T>
    std::optional<T> check_add(T a, T b);
    
public:
    // PC initial value should be at the beginning of the BIOS.
    uint32_t pc =  0xbfc00000;
    
    // Cop0; register 12; Status Register
    uint32_t sr = 0;
    
    // Load delay slot emulation.
    // Contains output of the current instruction
    RegisterIndex outRegs[32];
    
    // Load initiated by the current instruction
    Load load = {{0}, 0};
    
    // General purpose registers
    // First entry must always contain a 0.
    RegisterIndex regs[32];
    
private:
    Instruction* nextInstruction = new Instruction(0x0); //NOP
    
    Interconnect* interconnect;
};