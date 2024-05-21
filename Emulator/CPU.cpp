﻿#include "CPU.h"
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
    
    Instruction* instruction = nextInstruction;
    nextInstruction = new Instruction(load32(pc));
    
    // increment the PC
    pc = nextpc;
    nextpc = wrappingAdd(pc, 4);
    
    RegisterIndex reg = load.regIndex;
    auto val   = load.value;
    set_reg(reg, val);
    
    // Rest load register
    load = {{0}, 0};

    // If the last instruction was a branch then we're in the delay slot
    delaySlot = branchSlot;
    branchSlot = false;
    
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
    
    std::cerr << "Processing; " + getDetails(instruction.op) << " = " <<  std::to_string(instruction.func()) << " PC; " << pc << '\n';
    
    switch (instruction.func()) {
    case 0b000000:
        decodeAndExecuteSubFunctions(instruction);
        break;
    case 0b000001:
        opbxx(instruction);
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
    case 0b101001:
        opsh(instruction);
        break;
    case 0b101000:
        opsb(instruction);
        break;
    case 0b100011:
        oplw(instruction);
        break;
    case 0b100000:
        oplb(instruction);
        break;
    case 0b100100:
        oplbu(instruction);
        break;
    case 0b000010:
        opj(instruction);
        break;
    case 0b000011:
        opjal(instruction);
        break;
    case 0b001010:
        opslti(instruction);
        break;
    case 0b001011:
        opsltiu(instruction);
        break;
    case 0b001001:
        addiu(instruction);
        break;
    case 0b001000:
        addi(instruction);
        break;
    case 0b001100:
        opandi(instruction);
        break;
    case 0b010000:
        opcop0(instruction);
        break;
    case 0b000101:
        opbne(instruction);
        break;
    case 0b000100:
        opbeq(instruction);
        break;
    case 0b000111:
        opbgtz(instruction);
        break;
    case 0b000110:
        opblez(instruction);
        break;
    default:
        //printf("Unhandled CPU instruction at 0x%08x = 0x%08x\n ", instruction.op, instruction.func().reg);
        std::cerr << "Unhandled instruction(CPU): " << getDetails(instruction.func()) << " = " << instruction.func() << '\n';
        throw std::runtime_error("Unhandled instruction(CPU): " + getDetails(instruction.op) + " = " + std::to_string(instruction.op));
    }
}

void CPU::decodeAndExecuteSubFunctions(Instruction& instruction) {
    std::cerr << "Sub Processing; " + getDetails(instruction.op) << " / " <<  getDetails(instruction.subfunction()) << '\n';
    
    switch (instruction.subfunction()) {
    case 0b000000:
        opsll(instruction);
        break;
    case 0b000011:
        opsra(instruction);
        break;
    case 0b000010:
        opsrl(instruction);
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
    case 0b101010:
        opslt(instruction);
        break;
    case 0b001000:
        opjr(instruction);
        break;
    case 0b001001:
        opjalr(instruction);
        break;
    case 0b100000:
        add(instruction);
        break;
    case 0b100001:
        addu(instruction);
        break;
    case 0b011010:
        opdiv(instruction);
        break;
    case 0b011011:
        opdivu(instruction);
        break;
    case 0b010000:
        opmfhi(instruction);
        break;
    case 0b010001:
        opmthi(instruction);
        break;
    case 0b010010:
        opmflo(instruction);
        break;
    case 0b001100:
        opmtlo(instruction);
        break;
    case 0b100011:
        opsubu(instruction);
        break;
    case 0b100100:
        opand(instruction);
        break;
    default:
        throw std::runtime_error("Unhandled sub instruction: " + getDetails(instruction.op));
        
        break;
    }
}

void CPU::opsll(Instruction& instruction) {
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
    uint32_t v = reg(t).reg << i;
    
    set_reg(d, v);
}

void CPU::opsra(Instruction& instruction) {
    RegisterIndex i = instruction.shift();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    
    uint32_t v = static_cast<uint32_t>(reg(t)) >> i;
    
    set_reg(d, static_cast<uint32_t>(v));
}

void CPU::opsrl(Instruction& instruction) {
    auto i = instruction.shift();
    auto t = instruction.t();
    auto d = instruction.d();
    
    uint32_t v = reg(t) >> i;
    
    set_reg(d, v);
}

void CPU::opsltiu(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto s = instruction.s();
    auto t = instruction.t();

    uint32_t v = reg(s) < i;

    set_reg(t, static_cast<uint32_t>(v));
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
    int32_t i = static_cast<int32_t>(instruction.imm_se());
    RegisterIndex s = instruction.s(); 
    RegisterIndex t = instruction.t();
    
    int32_t result = static_cast<int32_t>(reg(s) & 0xFFFFFFFF) < i;
    
    printf("Setting %x as %x\n", t.reg, result);
    set_reg(t, static_cast<uint32_t>(result));
}

void CPU::opslt(Instruction& instruction) {
    auto d = instruction.d();
    auto sReg = instruction.s();
    auto tReg = instruction.t();
    
    int32_t s = static_cast<int32_t>(reg(sReg));
    int32_t t = static_cast<int32_t>(reg(tReg));
    
    auto v = s < t;
    
    set_reg(d, static_cast<uint32_t>(v));
}

// Store word
void CPU::opsw(Instruction& instruction) {
    // Can't write if we are in cache isolation mode!
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-word while cache is isolated!\n";
        
        return;
    }
    
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    uint32_t v    = reg(t).reg;
    
    store32(addr, v);
}

// Store halfword
void CPU::opsh(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store while cache is isolated!\n";
        
        return;
    }
    
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // Don't really need static cast here, but it's easier to read
    // or at least.. Understand what is happening
    uint16_t v    = static_cast<uint16_t>(reg(t));
    
    store16(addr, v);
}

void CPU::opsb(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store while cache is isolated!\n";
        
        return;
    }
    
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    uint8_t v    = static_cast<uint8_t>(reg(t));
    
    store8(addr, v);
}

// Load word
void CPU::oplw(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
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

// Load byte
void CPU::oplb(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();
    
    RegisterIndex addr = wrappingAdd(reg(s), i);
    int8_t v = static_cast<int8_t>(load8(addr));
    
    // Put the load in the delay slot
    load = {t, static_cast<uint32_t>(v)};
}

void CPU::oplbu(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex t = instruction.t();
    RegisterIndex s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    uint8_t v = load8(addr);
    
    load = {t, static_cast<uint32_t>(v)};
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
    int32_t i = static_cast<int32_t>(instruction.imm_se());
    RegisterIndex t = instruction.t();
    RegisterIndex sreg = instruction.s();
    
    int32_t s = static_cast<int32_t>(reg(sreg));
    
    // Check if an overflow occurs
    std::optional<int32_t> v = check_add<int32_t>(s, i);
    if(!v.has_value()) {
        // Overflow!
        std::cout << "ADDi overflow\n";
        exception(Overflow);
    }
    
    set_reg(t, static_cast<uint32_t>(v.value()));
}

void CPU::opdiv(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();

    int32_t n = static_cast<int32_t>(reg(s));
    int32_t d = static_cast<int32_t>(reg(t));

    if(d == 0) {
        // Division by zero.. Please don't
        hi = static_cast<uint32_t>(n);

        if(n >= 0)
            lo = 0xffffffff;
        else
            lo = 1;
    } else if(static_cast<uint32_t>(n) == 0x80000000 && d == -1) {
        // Result is not representable in a 32bit signed integer
        hi = 0;
        lo = 0x80000000;
    } else {
        hi = static_cast<uint32_t>((n % d));
        lo = static_cast<uint32_t>(n / d);
    }
}

void CPU::opdivu(Instruction& instruction) {
    auto s = instruction.s();
    auto t = instruction.t();
    
    auto n = reg(s);
    auto d = reg(t);
    
    if(d == 0) {
        // Division by zero..
        hi = n;
        lo = 0xffffffff;
    } else {
        hi = n % d;
        lo = n / d;
    }
}

void CPU::opmfhi(Instruction& instruction) {
    auto d = instruction.d();
    
    set_reg(d, hi);
}

void CPU::opmthi(Instruction& instruction) {
    auto s = instruction.s();
    
    hi = reg(s);
}

void CPU::opmflo(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    
    set_reg(d, lo);
}

void CPU::opmtlo(Instruction& instruction) {
    auto s = instruction.s();
    
    lo = reg(s);
}

void CPU::opsubu(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    
    uint32_t v = wrappingAdd(reg(s), reg(t));
    
    set_reg(d, v);
}

void CPU::opand(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    RegisterIndex v = reg(s) & reg(t);
    
    set_reg(d, v);
}

void CPU::opandi(Instruction& instruction){
    RegisterIndex i = instruction.imm();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    RegisterIndex v = reg(s) & i;
    
    set_reg(t, v);
}

void CPU::opcop0(Instruction& instruction) {
    // Formation goes as:
    // 0b0100nn where nn is the coprocessor number.
    
    switch (instruction.copOpcode()) {
    case 0b00100: // COP0
        opmtc0(instruction);
        break;
    case 0b000000:
        opmfc0(instruction);
        break;
    case 0b10000:
        oprfe(instruction);
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
    
    uint32_t v = cpur;
    
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
        // cause register
        if(v != 0) {
            throw std::runtime_error("Unhandled write to cop0 register " + std::to_string(copr));
        }
        
        break;
    default:
        std::cout << "Unhandled cop0 register " << copr << "\n";
        throw std::runtime_error("Unhandled cop0 register " + std::to_string(copr));
        
        break;
    }
    
    load = {cpur, v};
}

void CPU::opmfc0(Instruction& instruction) {
    auto cpur = instruction.t();
    uint32_t copr = instruction.d().reg;
    
    switch (copr) {
    case 12:
        sr = copr;
        break;
    case 13:
        cause = copr;
        break;
    case 14:
        epc = copr;
        break;
    default:
        throw std::runtime_error("Unhandled read from cop0r " + copr);
        
        break;
    }
}

void CPU::oprfe(Instruction& instruction) {
    if(instruction.op & 0x3f != 0b010000) {
        throw std::runtime_error("Invalid cop0 instruction; " + instruction.op);
    }

    auto mode = sr & 0x3f;
    sr &= !0x3f;
    sr |= mode >> 2;
}

void CPU::exception(Exception cause) {
    // Determine the exception handler address based on the 'BEV' bit
    uint32_t handler = (sr & (1 << 22)) != 0 ? 0xbfc00180 : 0x80000080;
    
    // Shift bits [5:0] of 'SR' two places to the left
    uint32_t mode = sr & 0x3F;
    sr &= ~0x3F;
    sr |= (mode << 2) & 0x3F;
    
    // Update 'CAUSE' register with the exception code (bits [6:2])
    this->cause = static_cast<uint32_t>(cause) << 2;
    
    // Save current instruction address in 'EPC'
    epc = currentpc;

    if(delaySlot) {
        // When an exception occurs in a delay slot 'EPC' points
        // to the branch instruction and bit 31 of 'CAUSE' is set.
        epc -= 4; // WrappingSub(4) ;-}
        //this->cause |= (1 << 31);
        // C++ is too epic :D
        this->cause = (static_cast<uint32_t>(cause) << 2) | (this->cause & 0x3FC00000);
    }

    // Exceptions don’t have a branch delay, we jump directly into the handler
    pc = handler;
    nextpc = pc + 4;//wrappingAdd(pc, 4);
}

void CPU::opSyscall(Instruction& instruction) {
    exception(Exception::Syscall);
}

void CPU::opj(Instruction& instruction) {
    uint32_t i = instruction.imm_jump();
    
    pc = (pc & 0xf0000000) | (i << 2);
}

void CPU::opjr(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    
    pc = reg(s);
}

void CPU::opjal(Instruction& instruction) {
    // Store return address in £31($ra)
    set_reg(31, pc);
    
    opj(instruction);
}

void CPU::opjalr(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();

    set_reg(d, pc);
    pc = reg(s);
}

void CPU::opbxx(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    
    bool isbgez = (instruction.op >> 16) & 1;
    bool islink = ((instruction.op >> 20) & 1) != 0;
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    // Test "less than zero"
    uint32_t test (static_cast<uint32_t>(v) < 0);

    // If the test is "greater than or equal to zero" we need
    // to negate the comparison above since
    // ("a >= 0" <=> "!(a < 0)"). The xor takes care of that.
    test = test ^ isbgez;

    if(test != 0) {
        if(islink) {
            // Store return address in R31 of RA
            set_reg(31, pc);
        }
        
        branch(i);
    }
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

void CPU::opbeq(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    if(reg(s) == reg(t))
        branch(i);
}

void CPU::opbgtz(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v > 0) {
        branch(i);
    }
}

void CPU::opblez(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v <= 0) {
        branch(i);
    }
}

void CPU::branch(uint32_t offset) {
    // Offset immediate are always shifted to two places,
    // to the right as 'PC' addresses have to be aligned with 32 bits.
    offset = offset << 2;
    
    pc = wrappingAdd(pc, offset);
    
    // We need to compensate for the hardcoded
    // 'pc.wrapping_add(4)' in 'executeNextInstruction'
    
    // This was causing an error,
    // apparently swapping the values fixed it?
    // pc = wrappingSub(4, pc);
    // So.. This was causing another error,
    // after A LOT of debugging I noticed that,
    // pc was being rest back to '0' after BNE,
    // so I double-checked what the output was meant to be,
    // and it was supposed to be 4294967256.

    /**
     * So I did some digging on why wrappingSub wouldn't return 0 in this case;
        * 4294967260 is 0xFFFFFFEC in HEX, and when you perform the subraction,
        * so it'd be; 0xFFFFFFEC - 0x00000004 = 0xFFFFFFE8..
        * which isn't an overflow.
        
        * Another thing;
            * It seems that the wrapping subraction function only results in 0,
            * IF the subraction crosses the boundary of the minimum value(0)..
     */
    
    pc -= 4;
}

void CPU::add(Instruction& instruction) {
    int32_t s = static_cast<int32_t>(instruction.s().reg);
    int32_t t = static_cast<int32_t>(instruction.t().reg);
    RegisterIndex d = instruction.d();
    
    s = static_cast<int32_t>(reg(s));
    t = static_cast<int32_t>(reg(t));
    
    if(std::optional<int32_t> v = check_add<int32_t>(s, t)) {
        set_reg(d, static_cast<uint32_t>(v.value()));
    } else
        exception(Overflow);

    /*
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    uint32_t s_val = static_cast<uint32_t>(this->reg(s));
    uint32_t t_val = static_cast<uint32_t>(this->reg(t));
    uint32_t v;
    try {
        v = s_val + t_val;
    } catch (std::overflow_error& e) {
        throw std::runtime_error("ADD overflow");
    }
    this->set_reg(d, v);
     */
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

uint16_t CPU::load16(uint32_t addr) {
    return interconnect->load16(addr);
}

uint8_t CPU::load8(uint32_t addr) {
    return interconnect->load8(addr);
}

void CPU::store32(uint32_t addr, uint32_t val) {
    interconnect->store32(addr, val);
}

void CPU::store16(uint32_t addr, uint16_t val) {
    interconnect->store16(addr, val);
}

void CPU::store8(uint32_t addr, uint8_t val) {
    interconnect->store8(addr, val);
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
