#include "CPUTests.h"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CPU.h"

namespace {
constexpr uint32_t CodeBase = 0x80010000;
constexpr uint32_t DataBase = 0x80020000;
constexpr uint32_t ExceptionHandler = 0x80000080;

uint32_t r(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t shamt, uint32_t func) {
    return (rs << 21) | (rt << 16) | (rd << 11) | (shamt << 6) | func;
}

uint32_t i(uint32_t op, uint32_t rs, uint32_t rt, uint16_t imm) {
    return (op << 26) | (rs << 21) | (rt << 16) | imm;
}

uint32_t j(uint32_t op, uint32_t target) {
    return (op << 26) | ((target >> 2) & 0x03ffffff);
}

uint32_t cop(uint32_t op, uint32_t rs, uint32_t rt, uint32_t rd, uint32_t low = 0) {
    return (op << 26) | (rs << 21) | (rt << 16) | (rd << 11) | low;
}

std::string hex32(uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return out.str();
}

struct Runner {
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

    void expect(const std::string& name, bool ok, const std::string& detail = {}) {
        if (ok) {
            passed++;
            return;
        }

        failed++;
        failures.push_back(detail.empty() ? name : name + ": " + detail);
    }

    void expectEq(const std::string& name, uint32_t got, uint32_t wanted) {
        expect(name, got == wanted, "got " + hex32(got) + ", wanted " + hex32(wanted));
    }

    template <typename Fn>
    void test(const std::string& name, Fn&& fn) {
        try {
            fn();
            passed++;
        } catch (const std::exception& e) {
            failed++;
            failures.push_back(name + ": threw " + e.what());
        } catch (...) {
            failed++;
            failures.push_back(name + ": threw unknown exception");
        }
    }
};

struct CpuHarness {
    CPU cpu;

    CpuHarness() {
        cpu.branchSlot = false;
        cpu.jumpSlot = false;
        cpu.delaySlot = false;
        cpu.delayJumpSlot = false;
        cpu.currentpc = 0;
        cpu.pc = CodeBase;
        cpu.nextpc = CodeBase + 4;
        cpu.hi = 0;
        cpu.lo = 0;
        cpu.extraCycles = 0;
        cpu.loads[0] = {};
        cpu.loads[1] = {};
        cpu.paused = false;
        cpu._cop0.reset();

        for (uint32_t& reg : cpu.regs)
            reg = 0;

        cpu.interconnect._ram.reset();
        cpu.interconnect._scratchPad.reset();
        cpu.interconnect._cacheControl = CacheControl(0);
        cpu.interconnect.lastICacheMiss = false;
        for (auto& line : cpu.interconnect.icache)
            line = {};
    }

    void writeRam32(uint32_t addr, uint32_t value) {
        cpu.interconnect._ram.store<uint32_t>(addr, value);
    }

    uint32_t readRam32(uint32_t addr) {
        return cpu.interconnect._ram.load<uint32_t>(addr);
    }

    uint16_t readRam16(uint32_t addr) {
        return cpu.interconnect._ram.load<uint16_t>(addr);
    }

    uint8_t readRam8(uint32_t addr) {
        return cpu.interconnect._ram.load<uint8_t>(addr);
    }

    int run(uint32_t opcode) {
        writeRam32(cpu.pc, opcode);
        writeRam32(cpu.pc + 4, 0);
        return cpu.executeNextInstruction();
    }

    void commitLoad() {
        run(0);
    }

    uint32_t causeCode() const {
        return (cpu._cop0.cause >> 2) & 0x1f;
    }
};

void expectException(Runner& runner, const std::string& name, uint32_t opcode, Exception cause) {
    CpuHarness h;
    h.run(opcode);
    runner.expectEq(name + " cause", h.causeCode(), static_cast<uint32_t>(cause));
    runner.expectEq(name + " pc", h.cpu.pc, ExceptionHandler);
}

void testSpecial(Runner& runner) {
    runner.test("SPECIAL shifts", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0x80000001);
        h.run(r(0, 2, 3, 4, 0x00)); runner.expectEq("sll", h.cpu.reg(3), 0x00000010);
        h.run(r(0, 2, 4, 4, 0x02)); runner.expectEq("srl", h.cpu.reg(4), 0x08000000);
        h.run(r(0, 2, 5, 4, 0x03)); runner.expectEq("sra", h.cpu.reg(5), 0xf8000000);
        h.cpu.set_reg(6, 36);
        h.run(r(6, 2, 7, 0, 0x04)); runner.expectEq("sllv", h.cpu.reg(7), 0x00000010);
        h.run(r(6, 2, 8, 0, 0x06)); runner.expectEq("srlv", h.cpu.reg(8), 0x08000000);
        h.run(r(6, 2, 9, 0, 0x07)); runner.expectEq("srav", h.cpu.reg(9), 0xf8000000);
    });

    runner.test("SPECIAL logic and compare", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0xf0f0000f);
        h.cpu.set_reg(3, 0x0ff00ff0);
        h.run(r(2, 3, 4, 0, 0x24)); runner.expectEq("and", h.cpu.reg(4), 0x00f00000);
        h.run(r(2, 3, 5, 0, 0x25)); runner.expectEq("or", h.cpu.reg(5), 0xfff00fff);
        h.run(r(2, 3, 6, 0, 0x26)); runner.expectEq("xor", h.cpu.reg(6), 0xff000fff);
        h.run(r(2, 3, 7, 0, 0x27)); runner.expectEq("nor", h.cpu.reg(7), 0x000ff000);
        h.cpu.set_reg(8, 0xffffffff);
        h.cpu.set_reg(9, 1);
        h.run(r(8, 9, 10, 0, 0x2a)); runner.expectEq("slt", h.cpu.reg(10), 1);
        h.run(r(8, 9, 11, 0, 0x2b)); runner.expectEq("sltu", h.cpu.reg(11), 0);
    });

    runner.test("SPECIAL arithmetic", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 10);
        h.cpu.set_reg(3, 3);
        h.run(r(2, 3, 4, 0, 0x20)); runner.expectEq("add", h.cpu.reg(4), 13);
        h.run(r(2, 3, 5, 0, 0x21)); runner.expectEq("addu", h.cpu.reg(5), 13);
        h.run(r(2, 3, 6, 0, 0x22)); runner.expectEq("sub", h.cpu.reg(6), 7);
        h.run(r(2, 3, 7, 0, 0x23)); runner.expectEq("subu", h.cpu.reg(7), 7);
        h.cpu.set_reg(8, 0x7fffffff);
        h.cpu.set_reg(9, 1);
        h.run(r(8, 9, 10, 0, 0x20));
        runner.expectEq("add overflow", h.causeCode(), Overflow);
    });

    runner.test("SPECIAL hi lo", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0xfffffff0);
        h.cpu.set_reg(3, 3);
        h.run(r(2, 3, 0, 0, 0x18)); runner.expectEq("mult lo", h.cpu.lo, 0xffffffd0);
        h.run(r(0, 0, 4, 0, 0x12)); runner.expectEq("mflo", h.cpu.reg(4), 0xffffffd0);
        h.run(r(0, 0, 5, 0, 0x10)); runner.expectEq("mfhi", h.cpu.reg(5), 0xffffffff);
        h.run(r(2, 3, 0, 0, 0x19)); runner.expectEq("multu lo", h.cpu.lo, 0xffffffd0);
        h.cpu.set_reg(6, 0x12345678);
        h.cpu.set_reg(7, 0x9abcdef0);
        h.run(r(6, 0, 0, 0, 0x11)); runner.expectEq("mthi", h.cpu.hi, 0x12345678);
        h.run(r(7, 0, 0, 0, 0x13)); runner.expectEq("mtlo", h.cpu.lo, 0x9abcdef0);
        h.cpu.set_reg(8, 20);
        h.cpu.set_reg(9, 6);
        h.run(r(8, 9, 0, 0, 0x1a)); runner.expectEq("div lo", h.cpu.lo, 3); runner.expectEq("div hi", h.cpu.hi, 2);
        h.run(r(8, 9, 0, 0, 0x1b)); runner.expectEq("divu lo", h.cpu.lo, 3); runner.expectEq("divu hi", h.cpu.hi, 2);
    });

    runner.test("SPECIAL jumps and traps", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, CodeBase + 0x40);
        h.run(r(2, 0, 0, 0, 0x08));
        runner.expectEq("jr nextpc", h.cpu.nextpc, CodeBase + 0x40);

        CpuHarness h2;
        h2.cpu.set_reg(2, CodeBase + 0x80);
        h2.run(r(2, 0, 31, 0, 0x09));
        runner.expectEq("jalr link", h2.cpu.reg(31), CodeBase + 8);
        runner.expectEq("jalr nextpc", h2.cpu.nextpc, CodeBase + 0x80);

        expectException(runner, "syscall", r(0, 0, 0, 0, 0x0c), SysCall);
        expectException(runner, "break", r(0, 0, 0, 0, 0x0d), Break);
        expectException(runner, "illegal special", r(0, 0, 0, 0, 0x3f), IllegalInstruction);
    });
}

void testImmediateAndBranches(Runner& runner) {
    runner.test("Immediate arithmetic and logic", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0xfffffff0);
        h.run(i(0x08, 2, 3, 0x000f)); runner.expectEq("addi", h.cpu.reg(3), 0xffffffff);
        h.run(i(0x09, 2, 4, 0x0010)); runner.expectEq("addiu", h.cpu.reg(4), 0);
        h.run(i(0x0a, 2, 5, 0x0001)); runner.expectEq("slti", h.cpu.reg(5), 1);
        h.run(i(0x0b, 2, 6, 0x0001)); runner.expectEq("sltiu", h.cpu.reg(6), 0);
        h.run(i(0x0c, 2, 7, 0x00ff)); runner.expectEq("andi", h.cpu.reg(7), 0x000000f0);
        h.run(i(0x0d, 2, 8, 0x00ff)); runner.expectEq("ori", h.cpu.reg(8), 0xffffffff);
        h.run(i(0x0e, 2, 9, 0x00ff)); runner.expectEq("xori", h.cpu.reg(9), 0xffffff0f);
        h.run(i(0x0f, 0, 10, 0x1234)); runner.expectEq("lui", h.cpu.reg(10), 0x12340000);
    });

    runner.test("Jumps", [&] {
        CpuHarness h;
        h.run(j(0x02, 0x80010400));
        runner.expectEq("j nextpc", h.cpu.nextpc, 0x80010400);
        CpuHarness h2;
        h2.run(j(0x03, 0x80010800));
        runner.expectEq("jal link", h2.cpu.reg(31), CodeBase + 8);
        runner.expectEq("jal nextpc", h2.cpu.nextpc, 0x80010800);
    });

    runner.test("Branches", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 5);
        h.cpu.set_reg(3, 5);
        h.run(i(0x04, 2, 3, 4)); runner.expectEq("beq taken", h.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h2;
        h2.cpu.set_reg(2, 5);
        h2.cpu.set_reg(3, 6);
        h2.run(i(0x05, 2, 3, 4)); runner.expectEq("bne taken", h2.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h3;
        h3.cpu.set_reg(2, 0xffffffff);
        h3.run(i(0x06, 2, 0, 4)); runner.expectEq("blez taken", h3.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h4;
        h4.cpu.set_reg(2, 1);
        h4.run(i(0x07, 2, 0, 4)); runner.expectEq("bgtz taken", h4.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h5;
        h5.cpu.set_reg(2, 0xffffffff);
        h5.run(i(0x01, 2, 0x00, 4)); runner.expectEq("bltz taken", h5.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h6;
        h6.cpu.set_reg(2, 0);
        h6.run(i(0x01, 2, 0x01, 4)); runner.expectEq("bgez taken", h6.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h7;
        h7.cpu.set_reg(2, 0xffffffff);
        h7.run(i(0x01, 2, 0x10, 4));
        runner.expectEq("bltzal link", h7.cpu.reg(31), CodeBase + 8);
        runner.expectEq("bltzal target", h7.cpu.nextpc, CodeBase + 4 + 16);
        CpuHarness h8;
        h8.cpu.set_reg(2, 0);
        h8.run(i(0x01, 2, 0x11, 4));
        runner.expectEq("bgezal link", h8.cpu.reg(31), CodeBase + 8);
        runner.expectEq("bgezal target", h8.cpu.nextpc, CodeBase + 4 + 16);
    });
}

void testMemory(Runner& runner) {
    runner.test("Loads and stores", [&] {
        CpuHarness h;
        h.cpu.set_reg(1, DataBase);
        h.cpu.set_reg(2, 0x88776655);
        h.run(i(0x2b, 1, 2, 0)); runner.expectEq("sw", h.readRam32(DataBase), 0x88776655);
        h.run(i(0x29, 1, 2, 4)); runner.expectEq("sh", h.readRam16(DataBase + 4), 0x6655);
        h.run(i(0x28, 1, 2, 6)); runner.expectEq("sb", h.readRam8(DataBase + 6), 0x55);

        h.run(i(0x23, 1, 3, 0)); h.commitLoad(); runner.expectEq("lw", h.cpu.reg(3), 0x88776655);
        h.run(i(0x21, 1, 4, 4)); h.commitLoad(); runner.expectEq("lh", h.cpu.reg(4), 0x00006655);
        h.cpu.interconnect._ram.store<uint16_t>(DataBase + 8, 0x8001);
        h.run(i(0x21, 1, 5, 8)); h.commitLoad(); runner.expectEq("lh sign", h.cpu.reg(5), 0xffff8001);
        h.run(i(0x25, 1, 6, 8)); h.commitLoad(); runner.expectEq("lhu", h.cpu.reg(6), 0x00008001);
        h.cpu.interconnect._ram.store<uint8_t>(DataBase + 10, 0x80);
        h.run(i(0x20, 1, 7, 10)); h.commitLoad(); runner.expectEq("lb", h.cpu.reg(7), 0xffffff80);
        h.run(i(0x24, 1, 8, 10)); h.commitLoad(); runner.expectEq("lbu", h.cpu.reg(8), 0x00000080);
    });

    runner.test("Unaligned word helpers", [&] {
        CpuHarness h;
        h.cpu.set_reg(1, DataBase);
        h.cpu.set_reg(2, 0x11223344);
        h.writeRam32(DataBase, 0xaabbccdd);
        h.run(i(0x22, 1, 3, 1)); h.commitLoad();
        runner.expectEq("lwl", h.cpu.reg(3), 0xccdd0000);
        h.cpu.set_reg(4, 0x55667788);
        h.run(i(0x26, 1, 4, 2)); h.commitLoad();
        runner.expectEq("lwr", h.cpu.reg(4), 0x5566aabb);
        h.cpu.set_reg(5, 0x11223344);
        h.writeRam32(DataBase + 8, 0xaabbccdd);
        h.run(i(0x2a, 1, 5, 9));
        runner.expectEq("swl", h.readRam32(DataBase + 8), 0xaabb1122);
        h.writeRam32(DataBase + 12, 0xaabbccdd);
        h.run(i(0x2e, 1, 5, 14));
        runner.expectEq("swr", h.readRam32(DataBase + 12), 0x3344ccdd);
    });

    expectException(runner, "lw misaligned", i(0x23, 1, 2, 1), LoadAddressError);
    expectException(runner, "sw misaligned", i(0x2b, 1, 2, 1), StoreAddressError);
    expectException(runner, "lh misaligned", i(0x21, 1, 2, 1), LoadAddressError);
    expectException(runner, "sh misaligned", i(0x29, 1, 2, 1), StoreAddressError);
}

void testCoprocessors(Runner& runner) {
    runner.test("COP0", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0x12345678);
        h.run(cop(0x10, 0x04, 2, 12)); runner.expectEq("mtc0 sr", h.cpu._cop0.sr, 0x12345678);
        h.run(cop(0x10, 0x00, 3, 12)); h.commitLoad(); runner.expectEq("mfc0 sr", h.cpu.reg(3), 0x12345678);
        h.cpu._cop0.sr = 0x3f;
        h.run(0x42000010); runner.expectEq("rfe", h.cpu._cop0.sr, 0x0f);
    });

    runner.test("COP2 data and control moves", [&] {
        CpuHarness h;
        h.cpu.set_reg(2, 0x12345678);
        h.run(cop(0x12, 0x04, 2, 0));
        h.run(cop(0x12, 0x00, 3, 0)); h.commitLoad();
        runner.expectEq("mtc2/mfc2", h.cpu.reg(3), 0x12345678);
        h.cpu.set_reg(4, 0x00001000);
        h.run(cop(0x12, 0x06, 4, 26));
        h.run(cop(0x12, 0x02, 5, 26)); h.commitLoad();
        runner.expectEq("ctc2/cfc2", h.cpu.reg(5), 0x00001000);
    });

    runner.test("LWC2 SWC2", [&] {
        CpuHarness h;
        h.cpu.set_reg(1, DataBase);
        h.writeRam32(DataBase, 0xcafebabe);
        h.run(i(0x32, 1, 0, 0));
        h.run(i(0x3a, 1, 0, 4));
        runner.expectEq("lwc2/swc2", h.readRam32(DataBase + 4), 0xcafebabe);
    });

    runner.test("GTE commands", [&] {
        const uint8_t commands[] = {
            0x01, 0x06, 0x0c, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x1b, 0x1c,
            0x1e, 0x20, 0x28, 0x29, 0x2a, 0x2d, 0x2e, 0x30, 0x3d, 0x3e, 0x3f
        };

        for (uint8_t command : commands) {
            CpuHarness h;
            h.cpu.set_reg(2, 0x00001000);
            h.run(cop(0x12, 0x06, 2, 26));
            h.run(0x4a000000 | command);
        }
    });

    expectException(runner, "COP1", cop(0x11, 0, 0, 0), CoprocessorError);
    expectException(runner, "COP3", cop(0x13, 0, 0, 0), CoprocessorError);
    expectException(runner, "LWC0", i(0x30, 0, 0, 0), CoprocessorError);
    expectException(runner, "LWC1", i(0x31, 0, 0, 0), CoprocessorError);
    expectException(runner, "LWC3", i(0x33, 0, 0, 0), CoprocessorError);
    expectException(runner, "SWC0", i(0x38, 0, 0, 0), CoprocessorError);
    expectException(runner, "SWC1", i(0x39, 0, 0, 0), CoprocessorError);
    expectException(runner, "SWC3", i(0x3b, 0, 0, 0), CoprocessorError);
}

void testIllegalPrimaryOpcodes(Runner& runner) {
    const uint8_t opcodes[] = {
        0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
        0x1c, 0x1d, 0x1e, 0x1f, 0x27, 0x2c, 0x2d, 0x2f,
        0x34, 0x35, 0x36, 0x37, 0x3c, 0x3d, 0x3e, 0x3f
    };

    for (uint8_t opcode : opcodes) {
        expectException(runner, "illegal primary " + std::to_string(opcode), i(opcode, 0, 0, 0), IllegalInstruction);
    }
}
}

bool CpuInstructionTests::runAll() {
    Runner runner;

    testSpecial(runner);
    testImmediateAndBranches(runner);
    testMemory(runner);
    testCoprocessors(runner);
    testIllegalPrimaryOpcodes(runner);

    std::cerr << "CPU instruction tests: " << runner.passed << " passed, " << runner.failed << " failed\n";

    for (const std::string& failure : runner.failures)
        std::cerr << "  " << failure << '\n';

    return runner.failed == 0;
}
