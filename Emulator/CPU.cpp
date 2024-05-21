#include "CPU.h"
#include "Instruction.h"

#include <array>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

//TODO Remove those
std::string getHex(uint32_t value) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(8) << std::hex << value;
    return ss.str();
}

std::string getBinary(uint32_t value) {
    std::string binary = std::bitset<32>(value).to_string();
    
    std::string shiftedBin = binary.substr(26);
    
    std::string results = std::bitset<6>(std::stoi(shiftedBin, nullptr, 2) & 0x3F).to_string();
    
    std::stringstream ss;
    // wrong hex values but who cares about those
    ss << "Binary; 0b" << results;
    
    return ss.str();
}

std::string getDetails(uint32_t value) {
    std::string hex = getHex(value);
    std::string binary = getBinary(value);

    return hex + " = " + binary;
}

/**
 * Byte - 8 bits
 * HalfWord - 16 bits or 2 bytes
 * Word - 32 bits or 4 bytes.
 */

void CPU::executeNextInstruction() {
    //regs[32] = { 0xdeadbeef};
    
    // R0 is "hardwired" to 0
    regs[0] = 0;
    
    /**
     * First amazing error, here I got "0x1300083c" which means..
     * I was reading the data in big-edian format amazingly..
     * To fix it, I just reversed how I was reading the BIOS.bin file.
     */
    
    RegisterIndex reg = load.regIndex;
    set_reg(reg, reg.reg);
    
    load = {{0}, 0};
    
    Instruction* instruction = nextInstruction;
    nextInstruction = new Instruction(load32(pc));
    
    // increment the PC
    pc = wrappingAdd(pc, 4);
    
    // Executes the instruction
    decodeAndExecute(*(instruction));
    
    // TODO; Make this as a function
    // regs = outRegs;
    std::copy(std::begin(outRegs), std::end(outRegs), std::begin(regs));
}

// page 92
//https://github.com/deadcore/playstation-emulator/blob/master/src/cpu/mod.rs
void CPU::decodeAndExecute(Instruction& instruction) {
    // Gotta decode the instructions using the;
    // Playstation R3000 processor
    // https://en.wikipedia.org/wiki/R3000
    
    std::cout << "Processingd; " + getDetails(instruction.op) << '\n';
    
    switch (instruction.func()) {
    case 0b000000:
        decodeAndExecuteSubFunctions(instruction);
        
        break;
    case 0b001111:
        oplui(instruction);
        break;
    case 0b001101:
        opori(instruction);
        break;
    case 0b101011:
        opsw(instruction);
        break;
    case 0b100011:
        oplw(instruction);
        break;
    case 0b000010:
        opj(instruction);
        break;
    case 0b001010:
        opslti(instruction);
        break;
    case 0b101001:
        opsh(instruction);
        break;
    case 0b001001:
        addiu(instruction);
        break;
    case 0b001000:
        addi(instruction);
        break;
    case 0b010000:
        opmtc0(instruction);
        break;
    case 0b000101:
        opbne(instruction);
        break;
    default:
        printf("Unhandled CPU instruction at 0x%08x\n", instruction.op);
        std::cerr << "Unhandled instruction(CPU): " << getDetails(instruction.func()) << " = " << instruction.func() << '\n';
        throw std::runtime_error("Unhandled instruction(CPU): " + getDetails(instruction.op));
    }
}

void CPU::decodeAndExecuteSubFunctions(Instruction& instruction) {
    switch (instruction.subfunction()) {
    case 0b000000:
        opsll(instruction);
        break;
    case 0b100101:
        opor(instruction);
        break;
    case 0b100111:
        opnor(instruction);
        break;
    case 0b101011:
        opsltu(instruction);
        break;
    case 0b100001:
        addu(instruction);
        break;
    default:
        throw std::runtime_error("Unhandled sub instruction: " + getDetails(instruction.op));
        
        break;
    }
}

void CPU::opsll(Instruction instruction) {
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
    uint32_t v = reg(t).reg << i;
    
    set_reg(d, v);
}

// Load Upper Immediate(LUI)
void CPU::oplui(Instruction& instruction) {
    uint32_t i = instruction.imm();
    uint32_t t = instruction.t();
    
    uint32_t v = i << 16;
    
    set_reg(t, v);
}

void CPU::opori(Instruction& instruction) {
    uint32_t i = instruction.imm();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t v = reg(s).reg | i;
    
    set_reg(t, v);
}

void CPU::opor(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    RegisterIndex v = reg(s) | reg(t);
    
    set_reg(d, v);
}

void CPU::opnor(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();

    RegisterIndex v = !(reg(s) | reg(t));

    set_reg(d, v);
}

void CPU::opsltu(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    RegisterIndex v = reg(s) < reg(t);
    
    set_reg(d, v); // V gets converted into a uint32
}

void CPU::opslti(Instruction& instruction) {
    int32_t immediate = static_cast<int32_t>(instruction.imm_se());
    RegisterIndex s = instruction.s(); 
    RegisterIndex t = instruction.t();
    
    int32_t result = static_cast<int32_t>(reg(s)) < immediate ? 1 : 0;
    
    set_reg(t, static_cast<uint32_t>(result));
}

// Store word
void CPU::opsw(Instruction& instruction) {
    // Can't write if we aren in cache isolation mode!
    if(sr & 0x10000 != 0) {
        std::cout << "Ignoring store-word while cache is isolated!";
        
        return;
    }
    
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s).reg, i);
    uint32_t v    = reg(t).reg;
    
    store32(addr, v);
}

// Load word
void CPU::oplw(Instruction instruction) {
    if(sr & 0x10000 != 0) {
        std::cout << "Ignoring store while cache is isolated!";
        
        return;
    }
    
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();
    
    RegisterIndex addr = wrappingAdd(reg(s), i);
    uint32_t v = load32(addr);
    
    // Put the load in the delay slot
    load = {t, v};
    //set_reg(t, v);
}

// Store halfword
void CPU::opsh(Instruction& instruction) {
    if(sr & 0x10000 != 0) {
        std::cout << "Ignoring store while cache is isolated!";
        
        return;
    }

    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();

    uint32_t addr = wrappingAdd(reg(s), i);
    uint16_t v    = static_cast<uint16_t>(reg(t));

    store16(addr, v);
}

// addiu $8, $zero, 0xb88
void CPU::addiu(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t v = wrappingAdd(reg(s), i);
    
    set_reg(t, v);
}

void CPU::addi(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    uint32_t s = instruction.s();
    
    s = reg(s);
    
    // Check if an overflow occours
    auto results = check_add<uint32_t>(s, i);
    if(!results.has_value()) {
        // Overflow!
        std::cout << "ADDi overflow\n";
        throw std::runtime_error("ADDi overflow\n");
    }
    
    uint32_t v = wrappingAdd(s, i);
    set_reg(t, v);
}

void CPU::opcop0(Instruction& instruction) {
    // Formation goes as:
    // 0b0100nn where nn is the coprocessor number.
    
    switch (instruction.copOpcode()) {
    case 0b00100: // COP0
        opmtc0(instruction);
        break;
    default:
        std::cout << "Unhandled instruction: " + getDetails(instruction.copOpcode()) << "\n";
        throw std::runtime_error("Unhandled COP instruction: " + getDetails(instruction.copOpcode()));
        
        break;        
    }
}

void CPU::opmtc0(Instruction& instruction) {
    // This basically makes it so that all the data,
    // goes to the cache instead of the main memory.
    RegisterIndex cpur = instruction.t();
    uint32_t copr = instruction.d().reg;
    
    RegisterIndex v = reg(cpur);
    
    switch (copr) {
    //Breakpoints registers for the future
    case 3: case 5: case 6: case 7: case 9: case 11:
        if(v != 0) {
            std::cout << "Unhandled write to cop0r{} " << copr << " val: " << v << "\n";
            throw std::runtime_error("Unhandled write to cop0r{} " + std::to_string(copr) + " val: " + std::to_string(v) + "\n");
        }
        
        break;
    case 12:
        sr = v;
        break;
    case 13:
        // case register
        if(v != 0) {
            throw std::runtime_error("Unhandled write to cop0 register " + copr);
        }
        
        break;
    default:
        std::cout << "Unhandled cop0 register " << copr << "\n";
        throw std::runtime_error("Unhandled cop0 register " + copr);
        
        break;
    }
}

void CPU::opj(Instruction& instruction) {
    uint32_t i = instruction.imm_jump();
    
    pc = (pc & 0xf0000000) | (i << 2);
}

void CPU::opbne(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    // Check if not equal
    if(reg(s) != reg(t)) {
        // Jump to the offset of i
        branch(i);
    }
}

void CPU::branch(uint32_t offset) {
    // Offset immediate are always shifted to two places,
    // to the right as 'PC' addresses have to be aligned with 32 bits.
    
    //TODO; BRANCHH!!
    offset = offset << 2;
    
    pc = wrappingAdd(pc, offset);
    
    // We need to compensate for the hardcoded
    // 'pc.wrapping_add(4)' in 'executeNextInstruction'
    pc = wrappingSub(4, pc);
}

void CPU::addu(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    
    RegisterIndex v = wrappingAdd(reg(s), reg(t));
    
    set_reg(d, v);
}

uint32_t CPU::load32(uint32_t addr) {
    return interconnect->load32(addr);
}

void CPU::store32(uint32_t addr, uint32_t val) {
    interconnect->store32(addr, val);
}

void CPU::store16(uint32_t addr, uint16_t val) {
    interconnect->store16(addr, val);
}

uint32_t CPU::wrappingAdd(uint32_t a, uint32_t b) {
    // Perform wrapping addition (wrap around upon overflow)
    return a + b < a ? UINT32_MAX : a + b;
}

uint32_t CPU::wrappingSub(uint32_t a, uint32_t b) {
    // Perform wrapping subtraction (wrap around upon underflow)
    return a - b > a ? 0 : a - b;
}

template <typename T>
std::optional<T> CPU::check_add(T a, T b) {
    if (b > std::numeric_limits<T>::max() - a) {
        // Overflow occurred
        return std::nullopt;
    }
    
    // No overflow, perform addition
    return a + b;
}
