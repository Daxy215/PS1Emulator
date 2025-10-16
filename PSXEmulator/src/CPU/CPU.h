#pragma once

#include <stdint.h>
#include <optional>
#include <unordered_set>

#include "Instruction.h"
#include "../Memory/interconnect.h"
#include "COP/Stolen/gte/gte.h"

class Instruction;

// Used for the load register
struct Load {
    uint32_t index;
    uint32_t value;
    
    Load(uint32_t reg, uint32_t val) : index(reg), value(val) {}
    Load() : index(32), value(0) {}
};

struct DisassembledInstruction {
    std::string text;
    uint32_t    opcode = 0;
    uint32_t    target = 0;
    bool        is_branch = false;
};

struct DisassemblerState {
    bool show = true;
    bool follow_pc = true;
    uint32_t view_center = 0;
    uint32_t prev_pc = 0;
    std::unordered_map<uint32_t, DisassembledInstruction> cache;
    char addrBuf[13] = {0};
    char filterBuf[64] = {0};
    int contextLines = 20;
    std::unordered_set<uint32_t> breakpoints;
    std::unordered_set<uint32_t> bookmarks;
    std::unordered_map<uint32_t, std::string> functionLabels;
    std::unordered_set<uint32_t> knownFunctions;
    bool show_address = true;
};

enum Exception {
    /// Interrupt Request
    Interrupt = 0x0,
    /// Address error on load
    LoadAddressError = 0x4,
    /// Address error on store
    StoreAddressError = 0x5,
    /// System call (caused by the SYSCALL opcode)
    SysCall = 0x8,
    /// Breakpoint (caused by the BREAK opcode)
    Break = 0x9,
    /// CPU encountered an unknown instruction
    IllegalInstruction = 0xa,
    /// Unsupported coprocessor operation
    CoprocessorError = 0xb,
    /// Arithmetic overflow
    Overflow = 0xc,
};

class CPU {
public:
    CPU() : currentpc(0), nextpc(pc + 4), regs() {}
    CPU(Interconnect interconnect) : currentpc(0), nextpc(pc + 4), regs{}, interconnect(std::move(interconnect)) {
        
    }
    
    int executeNextInstruction(bool print);
    int decodeAndExecute(Instruction& instruction);
    int decodeAndExecuteSubFunctions(Instruction& instruction);
    
    // TODO; THOSE ARE NOT COMPLETED!
    // TODO; Handle better functions,
    // TODO; the way functions are setup, is wrong
    void showDisassembler();
    DisassembledInstruction disassemble(Instruction& inst, uint32_t address);
    std::string getFunctionLabel(uint32_t addr);
    
    bool handleInterrupts(Instruction& instruction);
    
    // Instructions
    // TODO; Please move those in a different class future me!
    // Hell no
    
    // Shift Left Logical
    void opsll(Instruction& instruction);
    
    // Shift Left Logical Variable
    void opsllv(Instruction& instruction);
    
    // Shift Right Arithmetic
    void opsra(Instruction& instruction);
    
    // Shift Right Arithmetic Variable
    void opsrav(Instruction& instruction);
    
    // Shift Right Logical
    void opsrl(Instruction& instruction);
    
    // Shift Right Logical Variable
    void opsrlv(Instruction& instruction);
    
    // Set if Less Than Immediate Unsigned
    void opsltiu(Instruction& instruction);
    
    // Load Upper Immediate(LUI)
    void oplui(Instruction& instruction);
    
    // ori $8, $8, 0x243f
    // Puts the result of the "bitwise" or of $8 and 0x243f back into $8
    void opori(Instruction& instruction);
    
    // Bitwise or
    void opor(Instruction& instruction);
    
    // Bitwise not or
    void opnor(Instruction& instruction);
    
    // Bitwise Exclusive Or
    void opxor(Instruction& instruction);
    
    // Bitwise Exclusive or immediate
    void opxori(Instruction& instruction);
    
    // Set on less than unsigned
    void opsltu(Instruction& instruction);
    
    // Set on less than immediate
    void opslti(Instruction& instruction);
    
    // Set on Less Than signed
    void opslt(Instruction& instruction);
    
    // Store Word
    void opsw(Instruction& instruction);
    
    // Store Word Left (Little Endian)
    void opswl(Instruction& instruction);
    
    // Store Word Right (Little Endian)
    void opswr(Instruction& instruction);
    
    // Store Halfword
    void opsh(Instruction& instruction);
    
    // Store byte
    void opsb(Instruction& instruction);
    
    // Load word
    void oplw(Instruction& instruction);
    
    // Load Word Left (Could be unaligned - Little endian)
    void oplwl(Instruction& instruction);
    
    // Load Word Right little endian
    void oplwr(Instruction& instruction);
    
    // Store Word Left little endian
    //void opslw(Instruction& instruction);
    
    // Load Halfword signed
    void oplh(Instruction& instruction);
    
    // Load halfword unsigned
    void oplhu(Instruction& instruction);
    
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
    void opbqtz(Instruction& instruction);
    
    // Branch if less than or equal to zero
    void opbltz(Instruction& instruction);
    
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
    
    // Subtract Unsigned
    void opsubu(Instruction& instruction);
    
    // Subtract and check for signed overflow
    void opsub(Instruction& instruction);
    
    // Multiply Unsigned
    void opmultu(Instruction& instruction);
    
    // Multiply signed
    void opmult(Instruction& instruction);
    
    // Divide signed
    void opdiv(Instruction& instruction);
    
    // Divide Unsigned
    void opdivu(Instruction& instruction);
    
    // Move From HI
    void opmfhi(Instruction& instruction);
    
    // Move to HI
    void opmthi(Instruction& instruction);
    
    // Move from LO
    void opmflo(Instruction& instruction);
    
    // Move to LO
    void opmtlo(Instruction& instruction);
    
    // Load word in COP 0
    void oplwc0(Instruction& instruction);
    
    // Load word in COP 1
    void oplwc1(Instruction& instruction);
    
    // Load word in COP 2
    void oplwc2(Instruction& instruction);
    
    // Load word in COP 3
    void oplwc3(Instruction& instruction);
    
    // Store word in COP 0
    void opswc0(Instruction& instruction);
    
    // Store word in COP 1
    void opswc1(Instruction& instruction);
    
    // Store word in COP 2
    void opswc2(Instruction& instruction);
    
    // Store word in COP 3
    void opswc3(Instruction& instruction);
    
    // Bitwise And
    void opand(Instruction& instruction);
    
    // Bitwise And Immediate
    void opandi(Instruction& instruction);
    
    // Coprocessors handling
    int opcop0(Instruction& instruction);
    
    // Coprocessor 1 opcode (Doesn't exist on the PS1)
    int opcop1(Instruction& instruction);
    
    // Coprocessor 2w opcode (GTE)
    int opcop2(Instruction& instruction);
    
    // Coprocessor 3 opcode (Also doesn't exist on the PS1)
    int opcop3(Instruction& instruction);
    
    // Opcodes for coprocessor 0 - System Control
    // Also Coprocessor handling but only for COP0
    void opmtc0(Instruction& instruction);
    
    // Move from coprocessor 0
    void opmfc0(Instruction& instruction);
    
    // Return from exception
    void oprfe(Instruction& instruction);
    
    // Opcodes for coprocessor 2 - GTE
    void opmfc2(Instruction& instruction);
    void opcfc2(Instruction& instruction);
    void opmtc2(Instruction& instruction);
    void opctc2(Instruction& instruction);
    
    void opgte(Instruction& instruction);
    
    void exception(Exception cause);
    
    void opSyscall(Instruction& instruction);
    
    void opbreak(Instruction& instruction);
    
    void opillegal(Instruction& instruction);
    
    void checkForTTY();

    void reset();
    
    // Register related functions
    uint32_t reg(uint32_t index) {
        return regs[static_cast<size_t>(index)];
    }
    
    void set_reg(const uint32_t index, const uint32_t val) {
        // We need to always rest R0 to 0
        regs[0] = 0;
        
        if(index == 0)
            return;
        
        regs[index] = val;
        
        //regs[0] = {0};
        
        if(loads[0].index == index) {
            loads[0].index = 32;
        }
    }
    
    void setLoad(uint32_t index, uint32_t val) {
        if(index == 0)
            return;
        
        //load = {index, val};
        if(loads[0].index == index) {
            // Override previous write to the same register
            loads[0].index = 32;
        }
        
        loads[1] = {index, val};
    }
    
    // Memory related functions
    uint32_t load32(uint32_t addr);
    uint16_t load16(uint32_t addr);
    uint8_t  load8(uint32_t addr);
    
    void store32(uint32_t addr, uint32_t val);
    void store16(uint32_t addr, uint16_t val);
    void store8(uint32_t addr, uint8_t val);
    
    void handleCache(uint32_t addr, uint32_t val);
    
    // Helper functions
    static uint32_t wrappingAdd(uint32_t a, uint32_t b);
    static uint32_t wrappingSub(uint32_t a, uint32_t b);
    
    template<typename T>
    static std::optional<T> check_add(T a, T b);
    
    template<typename T>
    static std::optional<T> check_sub(T a, T b);
    
public:
    // Sets by the current instruction; if a branch occured,
    // and the next instruction will be in the delay slot.
    bool branchSlot = false;
    bool jumpSlot = false;
    
    // If the current instruction executes in the delay slot
    bool delaySlot = false;
    bool delayJumpSlot = false;
    
    // Used for setting the EPC in expections
    uint32_t currentpc;
    
    // PC initial value should be at the beginning of the BIOS
    uint32_t pc = 0xBFC00000;
    
    // Next value of the PC - Used to simulate the branch delay slot
    uint32_t nextpc;
    
    // High register for division remainder and multiplication high
    uint32_t hi = 0;
    
    // Low register for division quotient and multiplication low
    uint32_t lo = 0;
    
    // Load initiated by the current instruction
    //Load load = {{0}, 0};
    Load loads[2];
    
    // General purpose registers
    // First entry must always contain a 0.
    uint32_t regs[32];
    
    bool paused = false;
    bool stepRequested = false;
    bool stepUntilBranchTakenRequested = false;
    bool stepUntilBranchNotTakenRequested = false;
    
    DisassemblerState disasmState;
    
public:
    Interconnect interconnect;
    
    COP0 _cop0;
    
private:
    //COP2 _cop2;
    GTE gte;
};