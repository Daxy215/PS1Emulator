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
    void opsll(Instruction& instruction);

    // Shift Right Arithmetic
    void opsra(Instruction& instruction);
    
    // Load Upper Immediate(LUI)
    void oplui(Instruction& instruction);
    
    // ori $8, $8, 0x243f
    // Puts the result of the "bitwise" or of $8 and 0x243f back into $8
    void opori(Instruction& instruction);
    
    // Bitwise or
    void opor(Instruction& instruction);

    // Bitwise not or
    void opnor(Instruction& instruction);
    
    // Set on less than unsigned
    void opsltu(Instruction& instruction);
    
    // Set on less than immediate
    void opslti(Instruction& instruction);
    
    // Store Word
    void opsw(Instruction& instruction);
    
    // Store Halfword
    void opsh(Instruction& instruction);
    
    // Store byte
    void opsb(Instruction& instruction);
    
    // Load word
    void oplw(Instruction& instruction);
    
    // Load byte
    void oplb(Instruction& instruction);
    
    // Load Byte Unsigned
    void oplbu(Instruction& instruction);
    
    // Jump
    void opj(Instruction& instruction);
    
    // Jump register
    void opjr(Instruction& instruction);
    
    // Jump and link
    void opjal(Instruction& instruction);
    
    // Jump and link register
    void opjalr(Instruction& instruction);
    
    // Various branch instructions; BEGZ, BLTZ, BGEZAL, BLTZAL
    // Bits 16 and 20 are used to figure out which one to use.
    void opbxx(Instruction& instruction);
    
    // Branch if not equal
    void opbne(Instruction& instruction);
    
    // Branch if Equal
    void opbeq(Instruction& instruction);
    
    // Branch if greater than zero
    void opbgtz(Instruction& instruction);

    // Branch if less than or equal to zero
    void opblez(Instruction& instruction);
    
    // Branch to immediate value 'offset'
    void branch(uint32_t offset);

    // Add with expection on overflow
    void add(Instruction& instruction);
    
    // Add unsigned
    void addu(Instruction& instruction);
    
    // Add Immediate Unsigned
    void addiu(Instruction& instruction);
    
    // Same as addiu but generates an exception if it overflows
    void addi(Instruction& instruction);

    // Substract Unsigned
    void opsubu(Instruction& instruction);
    
    // Bitwise And
    void opand(Instruction& instruction);
    
    // Bitwise And Immediate
    void opandi(Instruction& instruction);
    
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
    uint16_t load16(uint32_t addr);
    uint8_t  load8(uint32_t addr);
    void store32(uint32_t addr, uint32_t val);
    void store16(uint32_t addr, uint16_t val);
    void store8(uint32_t addr, uint8_t val);
    
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