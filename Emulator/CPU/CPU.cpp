#include "CPU.h"
//#include "Instruction.h"

#include <bitset>
#include <iostream>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>

#include "../Utils/Bitwise.h"

/**
 * Byte - 8 bits or 1 byte
 * HalfWord - 16 bits or 2 bytes
 * Word - 32 bits or 4 bytes.
 */
void CPU::executeNextInstruction() {
    // R0 is "hardwired" to 0
    //regs[0] = 0;
    
    /**
     * First amazing error, here I got "0x1300083c" which means..
     * I was reading the data in big-edian format amazingly..
     * To fix it, I just reversed how I was reading the BIOS.bin file.
     */
    
    //uint32_t pc = this->pc;
    Emulator::Timers::Scheduler::tick(4);
    Instruction instruction = Instruction(interconnect.loadInstruction(pc));
    //std::cerr << (("Instruction; " + getInstructionName(instruction->op)).c_str()) << "\n";
    
    // Save the address of the current instruction to save in
    // 'EPC' in case of an exception.
    currentpc = pc;
    
    if(currentpc % 4 != 0) {
        exception(LoadAddressError);
        return;
    }
    
    // increment the PC
    this->pc = nextpc;
    nextpc = wrappingAdd(nextpc, 4);
    
    RegisterIndex reg = load.regIndex;
    auto val   = load.value;
    set_reg(reg, val);
    
    // Rest load register
    load = {{0}, 0};
    
    // If the last instruction was a branch then we're in the delay slot
    delaySlot = branchSlot;
    branchSlot = false;
    
    // Executes the instruction
    decodeAndExecute(instruction);
    
    // regs = outRegs;
    //std::copy(std::begin(outRegs), std::end(outRegs), std::begin(regs));
}

void CPU::decodeAndExecute(Instruction& instruction) {
    // Gotta decode the instructions using the;
    // Playstation R3000 processor
    // https://en.wikipedia.org/wiki/R3000
    
    //std::cerr << "Processing; " + getDetails(instruction.op) << " = " <<  std::to_string(instruction.func()) << " PC; " << pc << '\n';
    //printf("Processing CPU instruction at 0x%08x = 0x%08x\n ", instruction.op, instruction.func().reg);
    
    Emulator::Timers::Scheduler::tick(1);
    
    checkForTTY();
    
    switch (instruction.func()) {
        case 0b000000:
            decodeAndExecuteSubFunctions(instruction); // ALU operations (e.g., add, subtract, shift)
            break;
        case 0b001110:
            opxori(instruction); // XOR immediate
            break;
        case 0b000001:
            opbxx(instruction); // Branches and jumps
            break;
        case 0b001111:
            oplui(instruction); // Load Upper Immediate
            break;
        case 0b001101:
            opori(instruction); // OR immediate
            break;
        case 0b101011:
            opsw(instruction); // Store word
            break;
        case 0b101010:
            opswl(instruction); // Store word left
            break;
        case 0b101110:
            opswr(instruction); // Store word right
            break;
        case 0b101001:
            opsh(instruction); // Store halfword
            break;
        case 0b101000:
            opsb(instruction); // Store byte
            break;
        case 0b100011:
            oplw(instruction); // Load word
            break;
        case 0b100010:
            oplwl(instruction); // Load word left
            break;
        case 0b100110:
            oplwr(instruction); // Load word right
            break;
        case 0b100001:
            oplh(instruction); // Load halfword
            break;
        case 0b100101:
            oplhu(instruction); // Load halfword unsigned
            break;
        case 0b100000:
            oplb(instruction); // Load byte
            break;
        case 0b100100:
            oplbu(instruction); // Load byte unsigned
            break;
        case 0b000010:
            opj(instruction); // Jump
            break;
        case 0b000011:
            opjal(instruction); // Jump and link
            break;
        case 0b001010:
            opslti(instruction); // Set if less than immediate
            break;
        case 0b001011:
            opsltiu(instruction); // Set if less than immediate unsigned
            break;
        case 0b001001:
            addiu(instruction); // Add immediate unsigned
            break;
        case 0b001000:
            addi(instruction); // Add immediate
            break;
        case 0b001100:
            opandi(instruction); // AND immediate
            break;
        case 0b010000:
            opcop0(instruction); // Coprocessor 0 instructions
            break;
        case 0b010001:
            opcop1(instruction); // Coprocessor 1 instructions
            break;
        case 0b010010:
            opcop2(instruction); // Coprocessor 2 instructions
            break;
        case 0b010011:
            opcop3(instruction); // Coprocessor 3 instructions
            break;
        case 0b110000:
            oplwc0(instruction); // Load word coprocessor 0
            break;
        case 0b110001:
            oplwc1(instruction); // Load word coprocessor 1
            break;
        case 0b110010:
            oplwc2(instruction); // Load word coprocessor 2
            break;
        case 0b110011:
            oplwc3(instruction); // Load word coprocessor 3
            break;
        case 0b111000:
            opswc0(instruction); // Store word coprocessor 0
            break;
        case 0b111001:
            opswc1(instruction); // Store word coprocessor 1
            break;
        case 0b111010:
            opswc2(instruction); // Store word coprocessor 2
            break;
        case 0b111011:
            opswc3(instruction); // Store word coprocessor 3
            break;
        case 0b000101:
            opbne(instruction); // Branch if not equal
            break;
        case 0b000100:
            opbeq(instruction); // Branch if equal
            break;
        case 0b000111:
            opbqtz(instruction); // Branch if greater than zero
            break;
        case 0b000110:
            opbltz(instruction); // Branch if less than or equal to zero
            break;
        default:
            opillegal(instruction); // Illegal instruction
            printf("Unhandled CPU instruction at 0x%08x = 0x%08x = %x\n ", instruction.op, instruction.func().reg, instruction.op);
            std::cerr << "";
            //std::cerr << "Unhandled instruction(CPU): " << getDetails(instruction.func()) << " = " << instruction.func() << '\n';
            //throw std::runtime_error("Unhandled instruction(CPU): " + getDetails(instruction.op) + " = " + std::to_string(instruction.op));
            
            break;
    }
}

void CPU::decodeAndExecuteSubFunctions(Instruction& instruction) {
    //std::cerr << "Sub Processing; " + getDetails(instruction.op) << " / " <<  getDetails(instruction.subfunction()) << '\n';
    
    switch (instruction.subfunction()) {
    case 0b000000:
        opsll(instruction);
        break;
    case 0b100101:
        opor(instruction);
        break;
    case 0b000111:
        opsrav(instruction);
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
    case 0b001000:
        opjr(instruction);
        break;
    case 0b100100:
        opand(instruction);
        break;
    case 0b100000:
        add(instruction);
        break;
    case 0b001001:
        opjalr(instruction);
        break;
    case 0b100011:
        opsubu(instruction);
        break;
    case 0b000011:
        opsra(instruction);
        break;
    case 0b011010:
        opdiv(instruction);
        break;
    case 0b010010:
        opmflo(instruction);
        break;
    case 0b010000:
        opmfhi(instruction);
        break;
    case 0b000010:
        opsrl(instruction);
        break;
    case 0b011011:
        opdivu(instruction);
        break;
    case 0b101010:
        opslt(instruction);
        break;
    case 0b001100:
        opSyscall(instruction);
        break;
    case 0b001101:
        opbreak(instruction);
        break;
    case 0b010011:
        opmtlo(instruction);
        break;
    case 0b010001:
        opmthi(instruction);
        break;
    case 0b000100:
        opsllv(instruction);
        break;
    case 0b100110:
        opxor(instruction);
        break;
    case 0b011001:
        opmultu(instruction);
        break;
    case 0b000110:
        opsrlv(instruction);
        break;
    case 0b011000:
        opmult(instruction);
        break;
    case 0b100010:
        opsub(instruction);
        break;
    default:
        opillegal(instruction); // Illegal instruction
        printf("Unhandled sub instruction %0x8. Function call was: %x\n", instruction.op, instruction.subfunction().reg);
        std::cerr << "";
        break;
    }
}

// This is also stolen straight up from
// https://github.com/BluestormDNA/ProjectPSX/blob/master/ProjectPSX/Core/CPU.cs#L128
void CPU::handleInterrupts() {
    uint32_t load = interconnect.loadInstruction(pc);
    
    uint32_t instr = load >> 26;
    
    if(instr == 0x12) {
        // COP2 MTC2
        return;
    }
    
    // Check for interrupt
    if((IRQ::status & interconnect._irq.mask)) {
        this->cause |= 0x400;
    } else {
        this->cause &= ~0x400;
    }
    
    bool IEC = (sr & 0x1) == 1;
    uint8_t IM = static_cast<uint8_t>(sr >> 8) & 0xFF;
    uint8_t IP = static_cast<uint8_t>(this->cause >> 8) & 0xFF;
    
    if(IEC && (IM & IP) > 0) {
        exception(Interrupt);
    }
}

void CPU::opsll(Instruction& instruction) {
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
    uint32_t v = reg(t).reg << i;
    
    set_reg(d, v);
}

void CPU::opsllv(Instruction& instruction) {
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    // Shift amount is truncated to 5 bits
    uint32_t v = reg(t) << (reg(s) & 0x1F);
    
    set_reg(d, v);
}

void CPU::opsra(Instruction& instruction) {
    RegisterIndex i = instruction.shift();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    
    int32_t v = static_cast<int32_t>(reg(t)) >> i;
    
    set_reg(d, static_cast<uint32_t>(v));
}

void CPU::opsrav(Instruction& instruction) {
    auto d = instruction.d();
    auto s = instruction.s();
    auto t = instruction.t();
    
    // Shift amount is truncated to 5 bits
    // I just... Another issue here was that I was converting the entire result into an int32_t
    auto v = static_cast<int32_t>(reg(t)) >> (reg(s) & 0x1F);
    
    set_reg(d, static_cast<uint32_t>(v));
}

void CPU::opsrl(Instruction& instruction) {
    auto i = instruction.shift();
    auto t = instruction.t();
    auto d = instruction.d();
    
    uint32_t v = reg(t) >> i;
    
    set_reg(d, v);
}

void CPU::opsrlv(Instruction& instruction) {
    auto d = instruction.d();
    auto s = instruction.s();
    auto t = instruction.t();
    
    // Shift amount is truncated to 5 bits
    uint32_t v = reg(t) >> (reg(s) & 0x1F);
    
    set_reg(d, v);
}

void CPU::opsltiu(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto s = instruction.s();
    auto t = instruction.t();
    
    auto v = reg(s) < i;
    
    set_reg(t, static_cast<uint32_t>(v));
}

int b = 0;
// Load Upper Immediate(LUI)
void CPU::oplui(Instruction& instruction) {
    if(++b == 118949) {
        printf("");
    }
    
    auto i = instruction.imm();
    auto t = instruction.t();
    
    auto v = i << 16;
    
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
    auto d = instruction.d();
    auto s = instruction.s();
    auto t = instruction.t();
    
    auto v = reg(s) | reg(t);
    
    set_reg(d, v);
}

void CPU::opnor(Instruction& instruction) {
    auto d = instruction.d();
    auto s = instruction.s();
    auto t = instruction.t();
    
    // Was doing '!' instead of '~' :)
    auto v = ~(reg(s) | reg(t));
    
    set_reg(d, v);
}

void CPU::opxor(Instruction& instruction) {
    auto d = instruction.d();
    auto s = instruction.s();
    auto t = instruction.t();
    
    auto v = reg(s) ^ reg(t);
    
    set_reg(d, v);
}

void CPU::opxori(Instruction& instruction) {
    auto i = instruction.imm();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t v = reg(s) ^ i;
    
    set_reg(t, v);
}

void CPU::opsltu(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    
    bool v = reg(s) < reg(t);
    
    set_reg(d, static_cast<uint32_t>(v)); // V gets converted into a uint32
}

void CPU::opslti(Instruction& instruction) {
    int32_t i = static_cast<int32_t>(instruction.imm_se());
    RegisterIndex s = instruction.s(); 
    RegisterIndex t = instruction.t();
    
    uint32_t v = (static_cast<int32_t>(reg(s))) < i;
    
    set_reg(t, static_cast<uint32_t>(v));
}

void CPU::opslt(Instruction& instruction) {
    uint32_t d = instruction.d();
    uint32_t sReg = instruction.s();
    uint32_t tReg = instruction.t();
    
    int32_t s = static_cast<int32_t>(reg(sReg));
    int32_t t = static_cast<int32_t>(reg(tReg));
    
    bool v = s < t;
    
    set_reg(d, static_cast<uint32_t>(v));
}

// Store word
void CPU::opsw(Instruction& instruction) {
    // Can't write if we are in cache isolation mode!
    if((sr & 0x10000) != 0) {
        //std::cerr << "Ignoring store-word while cache is isolated!\n";
        
        return;
    }
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    if(addr % 4 == 0) {
        uint32_t v    = reg(t);
        
        store32(addr, v);
    } else {
        exception(StoreAddressError);
    }
}

void CPU::opswl(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = reg(s) + i;
    uint32_t v    = reg(t);
    
    uint32_t alignedAddr = addr & ~3;
    
    // Load the current value for the aligned word,
    // at the target address
    uint32_t curMem = load32(alignedAddr);
    uint32_t mem;
    
    switch (addr & 3) {
    case 0:
        mem = (curMem & 0xFFFFFF00) | (v >> 24);
        break;
    case 1:
        mem = (curMem & 0xFFFF0000) | (v >> 16);
        break;
    case 2:
        mem = (curMem & 0xFF000000) | (v >> 8);
        break;
    case 3:
        mem = (curMem & 0x00000000) | (v >> 0);
        break;
    default:
        throw std::runtime_error("Unreachable code!");
    }
    
    store32(alignedAddr, mem);
}

void CPU::opswr(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    uint32_t v    = reg(t);
    
    uint32_t alignedAddr = addr & ~3;
    
    // Load the current value for the aligned word,
    // at the target address
    uint32_t curMem = load32(alignedAddr);
    uint32_t mem;
    
    switch (addr & 3) {
    case 0:
        mem = (curMem & 0x0000000) | (v << 0);
        break;
    case 1:
        mem = (curMem & 0x000000FF) | (v << 8);
        break;
    case 2:
        mem = (curMem & 0x0000FFFF) | (v << 16);
        break;
    case 3:
        mem = (curMem & 0x00FFFFFF) | (v << 24);
        break;
    default:
        throw std::runtime_error("Unreachable code!");
    }
    
    store32(alignedAddr, mem);
}

// Store halfword
void CPU::opsh(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-halfword while cache is isolated!\n";
        
        return;
    }
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    if(addr % 2 == 0) {
        uint32_t v = reg(t);
        
        store16(addr, v);
    } else {
       exception(StoreAddressError); 
    }
}

void CPU::opsb(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-byte while cache is isolated!\n";
        
        return;
    }
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    uint32_t v    = reg(t);
    
    store8(addr, static_cast<uint8_t>(v));
}

// Load word
void CPU::oplw(Instruction& instruction) {
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store while cache is isolated!";
        
        return;
    }
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    auto addr = reg(s) +  i;
    
    if(addr % 4 == 0) {
        uint32_t v = load32(addr);
        
        // Put the load in the delay slot
        setLoad(t, v);
    } else {
        printf("wtf\n");
        //exception(LoadAddressError);
    }
}

void CPU::oplwl(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // This instruction bypasses the load delay restriction:
    // this instruction will merge the new contents,
    // with the value that is currently being loaded if needed.
    //uint32_t curv = outRegs[static_cast<size_t>(t)];
    uint32_t curv = reg(t);
    
    // Next we load the *aligned* word containing the first address byte
    uint32_t alignedAddr = addr & ~3;
    uint32_t alignedWord = load32(alignedAddr);
    
    // Depending on the address alignment we fetch, 1, 2, 3 or 4,
    // *most* significant bytes and put them in the target register.
    uint32_t v = addr & 3;
    
    switch (v) {
    case 0:
        v = (curv & 0x00FFFFFF) | (alignedWord << 24);
        break;
    case 1:
        v = (curv & 0x0000FFFF) | (alignedWord << 16);
        break;
    case 2:
        v = (curv & 0x000000FF) | (alignedWord << 8);
        break;
    case 3:
        v = (curv & 0x00000000) | (alignedWord << 0);
        break;
    default:
        throw std::runtime_error("Unreachable code!");
    }
    
    // Put the load in the delay slot
    //load = {t, v};
    setLoad(t, v);
}

void CPU::oplwr(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // This instruction bypasses the load delay restriction:
    // this instruction will merge the new contents,
    // with the value that is currently being loaded if needed.
    //uint32_t curv = outRegs[static_cast<size_t>(t)];
    uint32_t curv = reg(t);
    
    // Next we load the *aligned* word containing the first address byte
    uint32_t alignedAddr = addr & ~3;
    uint32_t alignedWord = load32(alignedAddr);
    
    // Depending on the address alignment we fetch, 1, 2, 3 or 4,
    // *most* significant bytes and put them in the target register.
    uint32_t v = addr & 3;
    
    switch (v) {
    case 0:
        v = (curv & 0x00000000) | (alignedWord >> 0);
        break;
    case 1:
        v = (curv & 0xFF000000) | (alignedWord >> 8);
        break;
    case 2:
        v = (curv & 0xFFFF0000) | (alignedWord >> 16);
        break;
    case 3:
        v = (curv & 0xFFFFFF00) | (alignedWord >> 24);
        break;
    default:
        throw std::runtime_error("Unreachable code!");
    }
    
    // Put the load in the delay slot
    //load = {t, v};
    setLoad(t, v);
}

void CPU::oplh(Instruction& instruction) {
    // Can't write if we are in cache isolation mode!
    if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-word while cache is isolated!\n";
        
        return;
    }
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    if(addr % 2 == 0) {
        // Cast as i16 to force sign extension
        // Had an issue here; I was making this as a uint32_t rather than int16_t
        int16_t v = static_cast<int16_t>(load16(addr));
        
        setLoad(t, static_cast<uint32_t>(v));
    } else {
        exception(LoadAddressError);
    }
}

void CPU::oplhu(Instruction& instruction) {
    uint32_t i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // Address must be 16bit aligned
    if(addr % 2 == 0) {
        uint16_t v = load16(addr);
        
        setLoad(t, static_cast<uint32_t>(v));
    } else {
        exception(LoadAddressError);
    }
}

// Load byte
void CPU::oplb(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    auto addr = wrappingAdd(reg(s), i);
    int8_t v = static_cast<int8_t>(load8(addr));
    
    // Put the load in the delay slot
    setLoad(t, static_cast<uint32_t>(v));
}

void CPU::oplbu(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = reg(s) + i;
    
    uint8_t v = load8(addr);
    
    // Put the load in the delay slot
    setLoad(t, static_cast<uint32_t>(v));
}

// addiu $8, $zero, 0xb88
void CPU::addiu(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t v = wrappingAdd(reg(s), i);
    
    set_reg(t, v);
}

void CPU::addi(Instruction& instruction) {
    int32_t i = static_cast<int32_t>(instruction.imm_se());
    uint32_t t = instruction.t();
    uint32_t sReg = instruction.s();
    
    int32_t s = static_cast<int32_t>(reg(sReg));
    
    if(std::optional<int32_t> v = check_add<int32_t>(s, i)) {
        set_reg(t, static_cast<uint32_t>(v.value()));
    } else {
        exception(Overflow);
    }
}

void CPU::opmultu(Instruction& instruction) {
    auto s = instruction.s();
    auto t = instruction.t();
    
    uint64_t a = static_cast<uint64_t>(reg(s));
    uint64_t b = static_cast<uint64_t>(reg(t));
    
    uint64_t v = a * b;
    
    hi = static_cast<uint32_t>(v >> 32);
    lo = static_cast<uint32_t>(v);
}

void CPU::opmult(Instruction& instruction) {
    auto s = instruction.s();
    auto t = instruction.t();
    
    int64_t a = static_cast<int64_t>(static_cast<int32_t>(reg(s)));
    int64_t b = static_cast<int64_t>(static_cast<int32_t>(reg(t)));
    
    uint64_t v = static_cast<uint64_t>((a * b));
    
    hi = static_cast<uint32_t>(v >> 32);
    lo = static_cast<uint32_t>(v);
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
        hi = static_cast<uint32_t>(n % d);
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

void CPU::oplwc0(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::oplwc1(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::oplwc2(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    auto addr = reg(s) + i;
    
    if((addr % 4) == 0) {
        uint32_t v = load32(addr);
        
        _cop2.setData(t, v);
    } else {
        //exception(LoadAddressError);
    }
}

void CPU::oplwc3(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::opswc0(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::opswc1(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::opswc2(Instruction& instruction) {
    auto s = instruction.s();
    auto t = instruction.t();
    auto i = instruction.imm_se();
    
    auto addr = wrappingAdd(reg(s), i);
    
    if(addr % 4 == 0) {
        auto v = _cop2.getData(t);
        
        store32(addr, v);
    } else {
        exception(LoadAddressError);
    }
}

void CPU::opswc3(Instruction& instruction) {
    // Not supported by this COP
    exception(CoprocessorError);
}

void CPU::opsubu(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    RegisterIndex t = instruction.t();
    RegisterIndex d = instruction.d();
    
    // I CANT BELIEVE I HAD THIS AS WRAPPINGADD AND NOT WRAPPINGSUB!!!!!
    uint32_t v = wrappingSub(reg(s), reg(t));
    //uint32_t v = reg(s) - reg(t);
    
    set_reg(d, v);
}

void CPU::opsub(Instruction& instruction) {
    auto sReg = instruction.s();
    auto tReg = instruction.t();
    auto d = instruction.d();
    
    int32_t s = static_cast<int32_t>(reg(sReg));
    int32_t t = static_cast<int32_t>(reg(tReg));
    
    // Ain't no way I'm doing check_add instead of check_sub
    if(std::optional<int32_t> v = check_sub<int32_t>(s, t)) {
        set_reg(d, static_cast<uint32_t>(v.value()));
    } else {
        exception(Overflow);
    }
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
    case 0b000000:
        opmfc0(instruction);
        break;
    case 0b00100:
        opmtc0(instruction);
        break;
    case 0b10000:
        oprfe(instruction);
        break;
    default:
        printf("");
        //std::cerr << "Unhandled COP0 instruction: " + std::to_string(instruction.copOpcode()) << "\n";
        //throw std::runtime_error("Unhandled COP instruction: " + std::to_string((instruction.copOpcode())));
    }
}

// Doesn't exists
void CPU::opcop1(Instruction& instruction) {
    exception(CoprocessorError);
    //throw std::runtime_error("Unhandled coprocessor 1 instructions\n");
}

void CPU::opcop2(Instruction& instruction) {
    auto opcode = instruction.copOpcode();
    
    if(opcode & 0x10) {
        // TODO; Handle command at instruction.op & 0x3f
        auto cmd = instruction.op & 0x3F;
        
        return;
    }
    
    switch (opcode) {
    case 0b000000:
        // Move from GTE to Data register
        opmfc2(instruction);
        break;
    case 0b000010:
        // From GTE to Control register
        opcfc2(instruction);
        break;
    case 0b00100:
        // Data Register to GTE
        opmtc2(instruction);
        break;
    case 0b00110:
        // Data Register to GTE Control register
        opctc2(instruction);
        break;
    default:
        std::cerr << "Unhandled COP2 instruction: " + std::to_string((instruction.copOpcode())) << "\n";
        throw std::runtime_error("Unhandled COP instruction: " + std::to_string((instruction.copOpcode())));
    }
}

// Doesn't exists
void CPU::opcop3(Instruction& instruction) {
    exception(CoprocessorError);
}

void CPU::opmtc0(Instruction& instruction) {
    // This basically makes it so that all the data,
    // goes to the cache instead of the main memory.
    RegisterIndex cpur = instruction.t();
    uint32_t copr = instruction.d().reg;
    
    uint32_t v = reg(cpur);
    
    switch (copr) {
    //Breakpoints registers for the future
    case 3: case 5: case 6: case 7: case 9: case 11:
        if(v != 0) {
            std::cout << "Unhandled write to cop0r{} " << copr << " val: " << v << "\n";
            throw std::runtime_error("Unhandled write to cop0r{} " + std::to_string(copr) + " val: " + std::to_string(v) + "\n");
        }
        
        break;
    case 12: {
        bool shouldInterrupt = (sr & 0x1) == 1;
        bool cur = (v & 0x1) == 1;
        
        sr = v;
        
        uint32_t IM = (v >> 8) & 0x3;
        uint32_t IP = (cause >> 8) & 0x3;
        
        if(!shouldInterrupt && cur && (IM & IP) > 0) {
            pc = nextpc;
            exception(Exception::Interrupt);
        }
        
        break;
    }
    case 13: {
        cause &= ~0x300;
        cause |= v & 0x300;
        
        break;
    }
    default:
        std::cout << "Unhandled cop0 register " << copr << "\n";
        throw std::runtime_error("Unhandled cop0 register " + std::to_string(copr));
    }
}

void CPU::opmfc0(Instruction& instruction) {
    auto cpur = instruction.t();
    uint32_t copr = instruction.d().reg;
    
    uint32_t v = 0;
    
    /**
     * cop0r0-r2   - N/A
     * cop0r3      - BPC - Breakpoint on execute (R/W)
     * cop0r4      - N/A
     * cop0r5      - BDA - Breakpoint on data access (R/W)
     * cop0r6      - JUMPDEST - Randomly memorized jump address (R)
     * cop0r7      - DCIC - Breakpoint control (R/W)
     * cop0r8      - BadVaddr - Bad Virtual Address (R)
     * cop0r9      - BDAM - Data Access breakpoint mask (R/W)
     * cop0r10     - N/A
     * cop0r11     - BPCM - Execute breakpoint mask (R/W)
     * cop0r12     - SR - System status register (R/W)
     * cop0r13     - CAUSE - Describes the most recently recognised exception (R)
     * cop0r14     - EPC - Return Address from Trap (R)
     * cop0r15     - PRID - Processor ID (R)
     * cop0r16-r31 - Garbage
     * cop0r32-r63 - N/A - None such (Control regs)
     */
    
    switch (copr) {
    case 6:
    case 7:
    case 8:
        break;
    case 12:
        v = sr;
        break;
    case 13:
        v = cause;
        break;
    case 14:
        v = epc;
        break;
    case 15:
        /*
         * cop0r15     - PRID - Processor ID (R)
         * 
         * SCPH-1001 (North America): 0x00000001
         * SCPH-7502 (Europe): 0x00000002
         */
        
        v = 0x00000001;
        break;
    default:
        throw std::runtime_error("Unhandled read from cop0r " + std::to_string(copr));
    }
    
    setLoad(cpur, v);
}

void CPU::oprfe(Instruction& instruction) {
    // There are other instructions with the same encoding but all
    // are virtual memory related And the Playstation doesnt't
    // implement them. Still, let's make sure we're not running
    // buggy code.
    if((instruction.op & 0x3f) != 0b010000) {
        throw std::runtime_error("Invalid cop0 instruction; " + instruction.op);
    }
    
    uint32_t mode = sr & 0x3F;
    sr &= ~0xF;
    sr |= mode >> 2;
}

void CPU::opmfc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = _cop2.getData(d);
    
    setLoad(t, v);
}

void CPU::opcfc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = _cop2.getControl(d);
    
    setLoad(t, v);
}

void CPU::opmtc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = reg(t);
    
    _cop2.setData(d, v);
}

void CPU::opctc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = reg(t);
    
    _cop2.setControl(d, v);
}

void CPU::opgte(Instruction& instruction) {
    throw std::runtime_error("Unhandled GTE operator\n");
}

void CPU::exception(const Exception exception) {
    // Shift bits [5:0] of 'SR' two places to the left
    uint32_t mode = sr & 0x3f;
    
    sr &= ~0x3f;
    sr |= (mode << 2) & 0x3f;
    
    // Update 'CAUSE' register with the exception code (bits [6:2])
    this->cause &= ~0x7C;
    this->cause |= static_cast<uint32_t>(exception) << 2;
    
    if(delaySlot) {
        // When an exception occurs in a delay slot 'EPC' points
        // to the branch instruction and bit 31 of 'CAUSE' is set.
        epc = wrappingSub(currentpc, 4);
        this->cause |= (1u << 31);
    } else {
        // Save current instruction address in 'EPC'
        epc = currentpc;
        this->cause &= ~(1u << 31);
    }
    
    // Determine the exception handler address based on the 'BEV' bit
    uint32_t handler = (sr & (1 << 22)) != 0 ? 0xbfc00180 : 0x80000080;
    
    // Exceptions don’t have a branch delay, we jump directly into the handler
    pc = handler;
    nextpc = wrappingAdd(pc, 4);
    
    if(exception != 8 && exception != 0)
        std::cerr << "EXCEPTION OCCURRED!!" << exception << "\n";
}

void CPU::opSyscall(Instruction& instruction) {
    exception(SysCall);
}

void CPU::opbreak(Instruction& instruction) {
    exception(Break);
}

void CPU::opillegal(Instruction& instruction) {
    printf("Illegal instruction %d %d PC; %d\n", instruction.func().reg, instruction.subfunction().reg, currentpc);
    //std::cerr << "Illegal instruction " << instruction.op << " PC; " << currentpc << "\n";
    exception(IllegalInstruction);
}

char prev = 'f';
int x = 0;

void CPU::checkForTTY() {
    //uint32_t pc = this->pc & 0x1FFFFFFF;
    
    if ((pc == 0x000000A0 || pc == 0x000000B0 || pc == 0x000000C0)) {
        //uint32_t r9 = regs[9];
        
        //if (r9 == 0x3C || r9 == 0x3D) {
            char ch = static_cast<char>(regs[4] & 0xFF);
            
            if(ch == 'n' && prev == ' ') {
                if(++x == 9)
                    printf("");
            }
            
            prev = ch;
            
            if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ') {
                std::cerr << ch;
            }
        //}
    }
}

void CPU::opj(Instruction& instruction) {
    uint32_t i = instruction.imm_jump();
    
    nextpc = (pc & 0xf0000000) | (i << 2);
    
    branchSlot = true;
}

void CPU::opjr(Instruction& instruction) {
    RegisterIndex s = instruction.s();
    
    nextpc = reg(s);
    
    branchSlot = true;
}

void CPU::opjal(Instruction& instruction) {
    // Store return address in £31($ra)
    uint32_t ra = nextpc;
    
    opj(instruction);
    
    set_reg(31, ra);
}

void CPU::opjalr(Instruction& instruction) {
    RegisterIndex d = instruction.d();
    RegisterIndex s = instruction.s();
    
    set_reg(d, nextpc);
    nextpc = reg(s);
    
    branchSlot = true;
}

void CPU::opbxx(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto s = instruction.s();
    
    uint32_t op = instruction.op;
    
    bool isbgez = (op >> 16) & 1;
    
    // It's not enough to test for bit 20 to see if we're supposed
    // to link, if any bit in the range [19:17] is set the link
    // doesn't take place And RA is left untouched.
    bool islink = ((op >> 17) & 0xF) == 0x8;
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    // Test "less than zero"
    auto test = (static_cast<uint32_t>(v < 0));
    
    // If the test is "greater than or equal to zero" we need
    // to negate the comparison above since
    // ("a >= 0" <=> "!(a < 0)"). The xor takes care of that.
    test = test ^ isbgez;
    
    if(islink) {
        // Store return address in R31 of RA
        set_reg(31, nextpc);
    }
    
    if(test != 0) {
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

void CPU::opbqtz(Instruction& instruction) {
    RegisterIndex i = instruction.imm_se();
    RegisterIndex s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v > 0) {
        branch(i);
    }
}

void CPU::opbltz(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v <= 0) {
        branch(i);
    }
}

void CPU::branch(uint32_t offset) {
    // Offset immediate are always shifted to two places,
    // to the right as 'PC' addresses have to be aligned with 32 bits.
    offset = offset << 2;
    
    //pc = wrappingAdd(pc, offset);
    nextpc = wrappingAdd(pc, offset);
    
    branchSlot = true;
}

int c = 0;

void CPU::add(Instruction& instruction) {
    // Test start
    if(++c == 972 + 174) {
        printf("");
    }
    
    auto sReg = instruction.s();
    auto tReg = instruction.t();
    auto d = instruction.d();
    
    int32_t s = static_cast<int32_t>(reg(sReg));
    int32_t t = static_cast<int32_t>(reg(tReg));
    
    if (std::optional<int32_t> v = check_add<int32_t>(s, t)) {
        set_reg(d, static_cast<uint32_t>(v.value()));
    } else {
        exception(Overflow);
    }
}

void CPU::addu(Instruction& instruction) {
    auto s = instruction.s();
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = wrappingAdd(reg(s), reg(t));
    
    set_reg(d, v);
}

uint32_t CPU::load32(uint32_t addr) {
    return interconnect.load<uint32_t>(addr);
    //return interconnect->load32(addr);
}

uint16_t CPU::load16(uint32_t addr) {
    return interconnect.load<uint16_t>(addr);
    //return interconnect->load16(addr);
}

uint8_t CPU::load8(uint32_t addr) {
    return interconnect.load<uint8_t>(addr);
    //return interconnect->load8(addr);
}

void CPU::store32(uint32_t addr, uint32_t val) {
    interconnect.store<uint32_t>(addr, val);
    //interconnect->store32(addr, val);
}

void CPU::store16(uint32_t addr, uint16_t val) {
    interconnect.store<uint16_t>(addr, val);
    //interconnect->store16(addr, val);
}

void CPU::store8(uint32_t addr, uint8_t val) {
    interconnect.store<uint8_t>(addr, val);
    //interconnect->store8(addr, val);
}

uint32_t CPU::wrappingAdd(uint32_t a, uint32_t b) {
    // Perform wrapping addition (wrap around upon overflow)
    //return a + b < a ? UINT32_MAX : a + b;
    
    // Turns out.. Mr C++ already does wrapping add
    // ty C++ so much.
    
    // I had to get a Rust IDE and learn it's basics to make sure
    // that they are both are outputting the same results,
    // and they were somehow after crying for days.
    return (a + b) & 0xFFFFFFFF;
    
    /*uint64_t temp = static_cast<uint64_t>(a) + b;
    
    if (temp > UINT32_MAX) {
        // Overflow occurred, wrap around
        temp -= UINT32_MAX + 1;
    }
    
    return static_cast<uint32_t>(temp);*/
    
    // Perform regular addition
    unsigned int sum = a + b;
    
    // Check for overflow
    if (sum < a || sum < b) {
        // Overflow occurred, wrap around
        return ~a; // This will effectively wrap around due to unsigned arithmetic
        //return sum;
    }
    
    return sum;
}

uint32_t CPU::wrappingSub(uint32_t a, uint32_t b) {
    // Perform wrapping subtraction (wrap around upon underflow)
    //return a - b > a ? 0 : a - b;
    return a - b;
    
    // Perform regular subtraction
    unsigned int diff = a - b;
    
    // Check for underflow
    if (diff > a) {
        // Underflow occurred, wrap around
        return (~b) + 1; // This will effectively wrap around due to unsigned arithmetic
    }
    
    return diff;
}

//https://stackoverflow.com/questions/3944505/detecting-signed-overflow-in-c-c
template <typename T>
std::optional<T> CPU::check_add(T lhs, T rhs) {
    if(lhs >= 0) {
        if(std::numeric_limits<T>::max() - lhs < rhs) {
            return std::nullopt;
        }
    } else {
        // Had std::numeric_limits<T>::max() instead of std::numeric_limits<T>::min()
        if(rhs < std::numeric_limits<T>::min() - lhs) {
            return std::nullopt;
        }
    }
    
    return lhs + rhs;
}

template <typename T>
std::optional<T> CPU::check_sub(T a, T b) {
    if (b >= 0) {
        if (a < std::numeric_limits<T>::min() + b) {
            return std::nullopt;
        }
    } else {
        if (a > std::numeric_limits<T>::max() + b) {
            return std::nullopt;
        }
    }
    
    return a - b;
}
