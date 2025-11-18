#include "CPU.h"
//#include "Instruction.h"

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <optional>
#include <stack>
#include <unordered_set>

#include "imgui.h"
#include "../Utils/Bitwise.h"

/**
 * Byte - 8 bits or 1 byte
 * HalfWord - 16 bits or 2 bytes
 * Word - 32 bits or 4 bytes.
 */
int CPU::executeNextInstruction() {
    /**
     * First amazing error, here I got "0x1300083c" which means..
     * I was reading the data in big-edian format amazingly..
     * To fix it, I just reversed how I was reading the BIOS.bin file.
     */
    
    // Save the address of the current instruction to save in
    // 'EPC' in case of an exception.
    currentpc = pc;
    
    if(currentpc % 4 != 0) {
        throw std::runtime_error("Shouldn't..?");
        //exception(LoadAddressError);
        
        return 0;
    }
    
    // If the last instruction was a branch then we're in the delay slot
    delaySlot = branchSlot;
    branchSlot = false;
    
    /**
     * I am currently cheating as I've been stuck for a very while,
     * and just comparing my cpu logs to avacods cpu's logs,
     * he updates bit 30 if an instruction actually jumped,
     * to a different pc address though doc says it's an unkown bit?
     * so, no idea what I'm really missing here?
     * 
     * 30    -      Not used (zero)=
     */
    delayJumpSlot = jumpSlot;
    jumpSlot = false;
    
    auto instruction = Instruction(interconnect.loadInstruction(pc));
    
    if(handleInterrupts(instruction)) {
        // An exception occurred, update instruction based on new PC
        instruction = Instruction(interconnect.loadInstruction(pc));
    }
    
    // increment the PC
    this->pc = nextpc;
    nextpc += 4;
    
    // Executes the instruction
    const int cycles = decodeAndExecute(instruction);
    
    // Shift load registers
    if(loads[0].index != 32)
        set_reg(loads[0].index, loads[0].value);
    
    loads[0] = loads[1];
    loads[1].index = 32;
    
    return cycles;
    
    //checkForTTY();
}

int CPU::decodeAndExecute(Instruction& instruction) {
    // Gotta decode the instructions using the;
    // Playstation R3000 processor
    // https://en.wikipedia.org/wiki/R3000
    
    // TODO; Handle cycles correctly - Currently all instructions are 2 cycles:
    // https://gist.github.com/allkern/b6ab6db6ac32f1489ad571af6b48ae8b
    
    static constexpr size_t TABLE_SIZE = 64;
    
    static std::array<std::function<int(CPU&, Instruction&)>, TABLE_SIZE> lookupTable = [] {
        std::array<std::function<int(CPU&, Instruction&)>, TABLE_SIZE> table = {};
        
        table.fill([](CPU& cpu, Instruction& inst) { cpu.opillegal(inst); return 1; });
        
        table[0b000000] = [](CPU& cpu, Instruction& inst) { return cpu.decodeAndExecuteSubFunctions(inst); }; // SPECIAL
        
        table[0b000001] = [](CPU& cpu, Instruction& inst) { cpu.opbxx(inst);  return 1; };      // Branch variants
        table[0b000010] = [](CPU& cpu, Instruction& inst) { cpu.opj(inst);    return 1; };      // j
        table[0b000011] = [](CPU& cpu, Instruction& inst) { cpu.opjal(inst);  return 1; };      // jal
        table[0b000100] = [](CPU& cpu, Instruction& inst) { cpu.opbeq(inst);  return 1; };      // beq
        table[0b000101] = [](CPU& cpu, Instruction& inst) { cpu.opbne(inst);  return 1; };      // bne
        table[0b000110] = [](CPU& cpu, Instruction& inst) { cpu.opbltz(inst); return 1; };      // bltz
        table[0b000111] = [](CPU& cpu, Instruction& inst) { cpu.opbqtz(inst); return 1; };      // bgtz
        
        table[0b001000] = [](CPU& cpu, Instruction& inst) { cpu.addi(inst); return 1; };        // addi
        table[0b001001] = [](CPU& cpu, Instruction& inst) { cpu.addiu(inst); return 1; };       // addiu
        table[0b001010] = [](CPU& cpu, Instruction& inst) { cpu.opslti(inst); return 1; };      // slti
        table[0b001011] = [](CPU& cpu, Instruction& inst) { cpu.opsltiu(inst); return 1; };     // sltiu
        table[0b001100] = [](CPU& cpu, Instruction& inst) { cpu.opandi(inst); return 1; };      // andi
        table[0b001101] = [](CPU& cpu, Instruction& inst) { cpu.opori(inst); return 1; };       // ori
        table[0b001110] = [](CPU& cpu, Instruction& inst) { cpu.opxori(inst); return 1; };      // xori
        table[0b001111] = [](CPU& cpu, Instruction& inst) { cpu.oplui(inst); return 1; };       // lui
        
        // Coprocessors — TODO;
        table[0b010000] = [](CPU& cpu, Instruction& inst) { return cpu.opcop0(inst); };         // cop0
        table[0b010001] = [](CPU& cpu, Instruction& inst) { return cpu.opcop1(inst); };         // cop1 (Not used in PS1)
        table[0b010010] = [](CPU& cpu, Instruction& inst) { return cpu.opcop2(inst); };         // GTE
        table[0b010011] = [](CPU& cpu, Instruction& inst) { return cpu.opcop3(inst); };         // Not used
        
        // Loads (2 cycles)
        table[0b100000] = [](CPU& cpu, Instruction& inst) { cpu.oplb(inst); return 2; };        // lb
        table[0b100001] = [](CPU& cpu, Instruction& inst) { cpu.oplh(inst); return 2; };        // lh
        table[0b100011] = [](CPU& cpu, Instruction& inst) { cpu.oplw(inst); return 2; };        // lw
        table[0b100100] = [](CPU& cpu, Instruction& inst) { cpu.oplbu(inst); return 2; };       // lbu
        table[0b100101] = [](CPU& cpu, Instruction& inst) { cpu.oplhu(inst); return 2; };       // lhu
        table[0b100010] = [](CPU& cpu, Instruction& inst) { cpu.oplwl(inst); return 2; };       // lwl
        table[0b100110] = [](CPU& cpu, Instruction& inst) { cpu.oplwr(inst); return 2; };       // lwr
        
        // Stores (1 cycle)
        table[0b101000] = [](CPU& cpu, Instruction& inst) { cpu.opsb(inst); return 1; };        // sb
        table[0b101001] = [](CPU& cpu, Instruction& inst) { cpu.opsh(inst); return 1; };        // sh
        table[0b101011] = [](CPU& cpu, Instruction& inst) { cpu.opsw(inst); return 1; };        // sw
        table[0b101010] = [](CPU& cpu, Instruction& inst) { cpu.opswl(inst); return 1; };       // swl
        table[0b101110] = [](CPU& cpu, Instruction& inst) { cpu.opswr(inst); return 1; };       // swr
        
        // Load/Store Coprocessor - TODO;
        table[0b110000] = [](CPU& cpu, Instruction& inst) { cpu.oplwc0(inst); return 2; };
        table[0b110001] = [](CPU& cpu, Instruction& inst) { cpu.oplwc1(inst); return 2; };
        table[0b110010] = [](CPU& cpu, Instruction& inst) { cpu.oplwc2(inst); return 2; };
        table[0b110011] = [](CPU& cpu, Instruction& inst) { cpu.oplwc3(inst); return 2; };
        table[0b111000] = [](CPU& cpu, Instruction& inst) { cpu.opswc0(inst); return 2; };
        table[0b111001] = [](CPU& cpu, Instruction& inst) { cpu.opswc1(inst); return 2; };
        table[0b111010] = [](CPU& cpu, Instruction& inst) { cpu.opswc2(inst); return 2; };
        table[0b111011] = [](CPU& cpu, Instruction& inst) { cpu.opswc3(inst); return 2; };
        
        return table;
    }();
    
    int cycles = 0;
    uint32_t func = instruction.func();
    
    if (TABLE_SIZE > func) {
        cycles = lookupTable[func](*this, instruction);
    } else {
        opillegal(instruction);
    }
    
    return cycles;
    
    if(!paused)
        return cycles;
    
    for (auto it = disasmState.cache.begin(); it != disasmState.cache.end(); ) {
        if (it->first > pc) {
            it = disasmState.cache.erase(it);
        } else {
            ++it;
        }
    }
    
    return cycles;
}

int CPU::decodeAndExecuteSubFunctions(Instruction& instruction) {
    switch (instruction.subfunction()) {
        case 0b000000:
            opsll(instruction);
            return 1;
        case 0b100101:
            opor(instruction);
            return 1;
        case 0b000111:
            opsrav(instruction);
            return 1;
        case 0b100111:
            opnor(instruction);
            return 1;
        case 0b101011:
            opsltu(instruction);
            return 1;
        case 0b100001:
            addu(instruction);
            return 1;
        case 0b001000:
            opjr(instruction);
            return 1;
        case 0b100100:
            opand(instruction);
            return 1;
        case 0b100000:
            add(instruction);
            return 1;
        case 0b001001:
            opjalr(instruction);
            return 1;
        case 0b100011:
            opsubu(instruction);
            return 1;
        case 0b000011:
            opsra(instruction);
            return 1;
        case 0b011010:
            opdiv(instruction);
            return 36; // Fixed latency for DIV
        case 0b010010:
            opmflo(instruction);
            return 1;
        case 0b010000:
            opmfhi(instruction);
            return 1;
        case 0b000010:
            opsrl(instruction);
            return 1;
        case 0b011011:
            opdivu(instruction);
            return 36; // Fixed latency for DIVU
        case 0b101010:
            opslt(instruction);
            return 1;
        case 0b001100:
            opSyscall(instruction);
            return 1;
        case 0b001101:
            opbreak(instruction);
            return 1;
        case 0b010011:
            opmtlo(instruction);
            return 1;
        case 0b010001:
            opmthi(instruction);
            return 1;
        case 0b000100:
            opsllv(instruction);
            return 1;
        case 0b100110:
            opxor(instruction);
            return 1;
        case 0b011001:
            opmultu(instruction);
            return 36; // Fixed latency for MULTU
        case 0b000110:
            opsrlv(instruction);
            return 1;
        case 0b011000:
            opmult(instruction);
            return 36; // Fixed latency for MULT
        case 0b100010:
            opsub(instruction);
            return 1;
        default:
            opillegal(instruction); // Illegal instruction
            printf("Unhandled sub instruction %0x8. Function call was: %x\n", instruction.op, instruction.subfunction());
            std::cerr << "";
            return 0;
    }
}

void CPU::showDisassembler() {
    if (!disasmState.show)
        return;
    
    ImGui::Begin("Disassembler", &disasmState.show);
    
    // === Top toolbar ===
    if (ImGui::Button(paused ? "Continue (F5)" : "Pause (F5)"))
        paused = !paused;
    
    ImGui::SameLine();
    
    if (ImGui::Button("Step (F10)")) {
        paused = true;
        stepRequested = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Step Until Branch Taken")) {
        paused = true;
        stepUntilBranchTakenRequested = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Step Until Branch Not Taken")) {
        paused = true;
        stepUntilBranchNotTakenRequested = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Skip Next Instruction")) {
        nextpc += 4;
    }
    
    ImGui::SameLine();
    ImGui::Checkbox("Follow PC", &disasmState.follow_pc);
    ImGui::SameLine();
    ImGui::Checkbox("Show Addresses", &disasmState.show_address);
    
    // === Go-to and filtering ===
    ImGui::InputText("Go to (hex)", disasmState.addrBuf, sizeof(disasmState.addrBuf),
                     ImGuiInputTextFlags_CharsHexadecimal);
    
    ImGui::SameLine();
    if (ImGui::Button("Go")) {
        uint32_t addr = strtoul(disasmState.addrBuf, nullptr, 16);
        disasmState.view_center = addr;
        disasmState.follow_pc = false;
    }
    
    ImGui::InputText("Filter (mnemonic/op)", disasmState.filterBuf, sizeof(disasmState.filterBuf));
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        disasmState.filterBuf[0] = '\0';
    }
    
    ImGui::Separator();
    
    // === Determine view center ===
    if (disasmState.follow_pc || pc != disasmState.prev_pc) {
        disasmState.view_center = pc;
        disasmState.prev_pc = pc;
    }
    
    // === Child region with clipper ===
    ImGui::BeginChild("##disasm_scroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Calculate total lines to display (we pad before and after center)
    const int linesAbove = disasmState.contextLines;
    const int linesBelow = disasmState.contextLines * 2;
    const uint32_t startAddr = disasmState.view_center - linesAbove * 4;
    const uint32_t totalLines = linesAbove + linesBelow + 1;
    
    ImGuiListClipper clipper;
    float item_height = ImGui::GetTextLineHeightWithSpacing();
    clipper.Begin(totalLines, item_height);
    
    bool insideFunction = false;
    uint32_t funcStart = 0;
    std::string currentFunctionLabel;
    
    while (clipper.Step()) {
        for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; line++) {
            uint32_t addr = startAddr + line * 4;
            
            if (addr < startAddr)
                continue;
            
            // Fetch or disassemble
            auto it = disasmState.cache.find(addr);
            DisassembledInstruction di;
            
            if (it != disasmState.cache.end()) {
                di = it->second;
            } else {
                uint32_t op = interconnect.loadInstruction(addr);
                Instruction inst(op);
                di = disassemble(inst, addr);
                disasmState.cache[addr] = di;
            }
            
            // Auto-discover function labels
            if (di.is_branch && di.target != 0)
                getFunctionLabel(di.target);
            
            // Function start label
            auto fit = disasmState.functionLabels.find(addr);
            
            if (fit != disasmState.functionLabels.end()) {
                insideFunction = true;
                funcStart = addr;
                currentFunctionLabel = fit->second;
                
                ImGui::NewLine();
                ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "Function_%s:", currentFunctionLabel.c_str());
                
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    ImGui::SetClipboardText(currentFunctionLabel.c_str());
                    std::cerr << "Copied\n";
                }
            }
            
            // Apply filter
            if (disasmState.filterBuf[0]) {
                std::string combined = di.text;
                
                std::string txt = di.text;
                std::transform(txt.begin(), txt.end(), txt.begin(), ::tolower);
                std::string filt(disasmState.filterBuf);
                std::transform(filt.begin(), filt.end(), filt.begin(), ::tolower);
                
                if (txt.find(filt) == std::string::npos)
                    continue;
            }
            
            // Highlight current PC
            bool isPC = (addr == pc);
            if (isPC)
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
            
            // Breakpoint marker
            if (disasmState.breakpoints.count(addr)) {
                ImGui::Text("●");
                ImGui::SameLine();
            } else {
                ImGui::Text(" ");
                ImGui::SameLine();
            }
            
            // Stop if current instruction has a breakpoint
            if (disasmState.breakpoints.count(pc)) {
                paused = true;
            }
            
            // Instruction (indented)
            //ImGui::Text("    %s", di.text.c_str());
            ImGui::Indent();
            
            // Address
            if (disasmState.show_address) {
                ImGui::Text("%08X:", addr);
                ImGui::SameLine();
            }
            
            if (di.is_branch) {
                ImGui::PushID((int)addr);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200,200,100,255));
                
                if (ImGui::Selectable(di.text.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    disasmState.view_center = di.target;
                    //disasmState.prev_pc = di.target;
                    disasmState.follow_pc = false;
                }
                
                ImGui::PopStyleColor();
                ImGui::PopID();
            } else {
                // fallback to original lexeme coloring
                std::istringstream iss(di.text);
                std::string word;
                bool first = true;
                float spacing = 8.0f;
                
                while (iss >> word) {
                    if (!first) ImGui::SameLine(0.0f, spacing);
                    if (first) {
                        ImGui::TextColored(ImVec4(0.8f,0.9f,1.0f,1.0f), "%s", word.c_str());
                        first = false;
                    } else if (word[0] == '$') {
                        ImGui::TextColored(ImVec4(0.4f,1.0f,0.4f,1.0f), "%s", word.c_str());
                    } else if (word.rfind("0x",0)==0 || std::isdigit(word[0])) {
                        ImGui::TextColored(ImVec4(1.0f,0.85f,0.4f,1.0f), "%s", word.c_str());
                    } else if (word == ";" || word == "//") {
                        std::string comment;
                        std::getline(iss, comment);
                        word += comment;
                        ImGui::SameLine(0.0f, spacing);
                        ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "%s", word.c_str());
                        break;
                    } else {
                        ImGui::Text("%s", word.c_str());
                    }
                }
            }
            
            ImGui::Unindent();
            
            // Right-click context menu
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                char popup_id[32];
                snprintf(popup_id, sizeof(popup_id), "ctxMenu_%08X", addr);
                ImGui::OpenPopup(popup_id);
            }
            
            char popup_id[32];
            snprintf(popup_id, sizeof(popup_id), "ctxMenu_%08X", addr);
            if (ImGui::BeginPopup(popup_id)) {
                if (ImGui::MenuItem(disasmState.breakpoints.count(addr) ? "Remove Breakpoint" : "Add Breakpoint")) {
                    if (disasmState.breakpoints.count(addr))
                        disasmState.breakpoints.erase(addr);
                    else
                        disasmState.breakpoints.insert(addr);
                }
                
                if (ImGui::MenuItem(disasmState.bookmarks.count(addr) ? "Remove Bookmark" : "Add Bookmark")) {
                    if (disasmState.bookmarks.count(addr))
                        disasmState.bookmarks.erase(addr);
                    else
                        disasmState.bookmarks.insert(addr);
                }
                
                ImGui::EndPopup();
            }
            
            // Tooltip
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Opcode: 0x%08X", di.opcode);
                ImGui::EndTooltip();
                
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "%08X: %s", addr, di.text.c_str());
                    
                    ImGui::SetClipboardText(buf);
                    std::cerr << "Copied\n";
                }
            }
            
            // Function end detection
            if (insideFunction) {
                bool endFunction = false;
                
                // Check if current instruction is a return (jr $ra)
                if (di.text.find("JR     ra(31)") != std::string::npos) {
                    endFunction = true;
                }
                // Check if next instruction isn't executable code
                else {
                    uint32_t nextAddr = addr + 4;
                    endFunction = !map::isExecutableAddress(nextAddr);
                }
                
                if (endFunction) {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "End of %s", currentFunctionLabel.c_str());
                    insideFunction = false;
                    ImGui::NewLine();
                }
            }
            
            // PC highlight
            if (isPC) {
                ImGui::PopStyleColor();
                
                if (disasmState.follow_pc)
                    ImGui::SetScrollHereY();
            }
        }
    }
    
    clipper.End();
    ImGui::EndChild();
    
    // === Bookmarks panel ===
    if (!disasmState.bookmarks.empty()) {
        ImGui::Separator();
        ImGui::Text("Bookmarks:");
        
        for (uint32_t addr : disasmState.bookmarks) {
            char buf[9];
            snprintf(buf, sizeof(buf), "%08X", addr);
            
            if (ImGui::SmallButton(buf)) {
                disasmState.view_center = addr;
                disasmState.follow_pc = false;
            }
            
            ImGui::SameLine();
        }
    }
    
    ImGui::End();
}

// TODO; THOSE ARE NOT COMPLETED!
DisassembledInstruction CPU::disassemble(Instruction& inst, uint32_t address) {
    DisassembledInstruction result;
    uint32_t opcode = inst.op;
    uint32_t funct = inst.subfunction();
    uint32_t rs = inst.s();
    uint32_t rt = inst.t();
    uint32_t rd = inst.d();
    uint32_t imm = inst.imm_se();
    uint32_t uimm = inst.imm();
    uint32_t jump_target = inst.imm_jump() << 2;
    uint32_t shift = inst.shift();
    
    result.opcode = opcode;
    
    std::stringstream ss;
    
    auto commentMemAccess = [&](uint32_t base, uint16_t imm) {
        uint32_t addr = regs[base] + (int16_t)imm;
        uint32_t masked = map::maskRegion(addr);
        uint32_t offset = 0;
        
        auto print = [&](const char* name) {
            ss << "    ; accessing " << name << " + 0x" << std::hex << offset;
        };
        
        if (map::BIOS.contains(masked, offset)) return print("BIOS");
        else if (map::GPU.contains(masked, offset)) return print("GPU");
        else if (map::SPU.contains(masked, offset)) return print("SPU");
        else if (map::DMA.contains(masked, offset)) return print("DMA");
        else if (map::PADMEMCARD.contains(masked, offset)) return print("PADMEMCARD");
        else if (map::IRQ_CONTROL.contains(masked, offset)) return print("IRQ_CONTROL");
        else if (map::CDROM.contains(masked, offset)) return print("CDROM");
        else if (map::TIMERS.contains(masked, offset)) return print("TIMER");
        else if (map::MDEC.contains(masked, offset)) return print("MDEC");
        else if (map::MEMCONTROL.contains(masked, offset)) return print("MEMCONTROL");
        else if (map::CACHECONTROL.contains(masked, offset)) return print("CACHECTRL");
        else if (map::SCRATCHPAD.contains(masked, offset)) return print("SCRATCHPAD");
        else if (map::RAM.contains(masked, offset)) return print("RAM");
        
        ss << "    ; unknown memory region at 0x" << std::hex << masked;
    };
    
    auto hex32 = [](uint32_t value) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
        return ss.str();
    };
    
    auto regname = [](uint32_t reg) {
        static const char* names[32] = {
            "zero(0)", "at(1)", "v0(2)", "v1(3)", "a0(4)", "a1(5)", "a2(6)", "a3(7)",
            "t0(8)", "t1(9)", "t2(10)", "t3(11)", "t4(12)", "t5(13)", "t6(14)", "t7(15)",
            "s0(16)", "s1(17)", "s2(18)", "s3(19)", "s4(20)", "s5(21)", "s6(22)", "s7(23)",
            "t8(24)", "t9(25)", "k0(26)", "k1(27)", "gp(28)", "sp(29)", "fp(30)", "ra(31)"
        };
        
        return names[reg];
    };
    
    auto comment2 = [&](uint32_t r1, uint32_t r2) {
        ss << "    ; " << regname(r1) << "=" << hex32(regs[r1]) << ", " << regname(r2) << "=" << hex32(regs[r2]);
    };
    
    auto commentBranch = [&](uint32_t r1, uint32_t r2, bool taken) {
        ss << "    ; " << regname(r1) << "=" << hex32(regs[r1]) << ", "
           << regname(r2) << "=" << hex32(regs[r2]) << " → " << (taken ? "TAKEN" : "NOT TAKEN");
    };
    
    auto commentJump = [&](uint32_t target) {
        ss << "    ; → " << getFunctionLabel(target);
    };
    
    bool taken = false;
    
    switch (inst.func()) {
        case 0x00: // SPECIAL
            switch (funct) {
            case 0x00: ss << "SLL    " << regname(rd) << ", " << regname(rt) << ", " << shift; break;
            case 0x02: ss << "SRL    " << regname(rd) << ", " << regname(rt) << ", " << shift; break;
            case 0x03: ss << "SRA    " << regname(rd) << ", " << regname(rt) << ", " << shift; break;
            case 0x04: ss << "SLLV   " << regname(rd) << ", " << regname(rt) << ", " << regname(rs); break;
            case 0x06: ss << "SRLV   " << regname(rd) << ", " << regname(rt) << ", " << regname(rs); break;
            case 0x07: ss << "SRAV   " << regname(rd) << ", " << regname(rt) << ", " << regname(rs); break;
            case 0x08: { // JR
                ss << "JR     " << regname(rs);
                ss << "    ; " << regname(rs) << "=" << hex32(regs[rs]);
                result.is_branch = true;
                break;
            }
            
            case 0x09: { // JALR
                ss << "JALR   " << regname(rd) << ", " << regname(rs);
                ss << "    ; " << regname(rs) << "=" << hex32(regs[rs]);
                result.is_branch = true;
                break;
            }
            
            case 0x0c: ss << "SYSCALL"; break;
            case 0x0d: ss << "BREAK"; break;
            case 0x10: ss << "MFHI   " << regname(rd); break;
            case 0x11: ss << "MTHI   " << regname(rs); break;
            case 0x12: ss << "MFLO   " << regname(rd); break;
            case 0x13: ss << "MTLO   " << regname(rs); break;
            case 0x18: ss << "MULT   " << regname(rs) << ", " << regname(rt); break;
            case 0x19: ss << "MULTU  " << regname(rs) << ", " << regname(rt); break;
            case 0x1a: ss << "DIV    " << regname(rs) << ", " << regname(rt); break;
            case 0x1b: ss << "DIVU   " << regname(rs) << ", " << regname(rt); break;
            case 0x1C: {
                switch (funct) {
                    case 0x02: ss << "MUL    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt); comment2(rs, rt); break;
                    case 0x20: ss << "CLZ    " << regname(rd) << ", " << regname(rs); comment2(rs, rs); break;
                    case 0x21: ss << "CLO    " << regname(rd) << ", " << regname(rs); comment2(rs, rs); break;
                    default: ss << "UNKNOWN SPECIAL2 (" << hex32(opcode) << ")"; break;
                }
                
                break;
            }
            case 0x1F: { // SPECIAL3
                switch (funct) {
                    case 0x00: ss << "EXT    " << regname(rt) << ", " << regname(rs) << ", " << std::dec << rs << ", " << ((rd >> 6) & 0x1F); comment2(rs, rs); break;
                    case 0x04: ss << "INS    " << regname(rt) << ", " << regname(rs) << ", " << std::dec << rs << ", " << ((rd >> 6) & 0x1F); comment2(rs, rs); break;
                    default: ss << "UNKNOWN SPECIAL3 (" << hex32(opcode) << ")"; break;
                }
                
                break;
            }
            case 0x20: { // ADD
                ss << "ADD    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x21: { // ADDU
                ss << "ADDU   " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x22: { // SUB
                ss << "SUB    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x23: { // SUBU
                ss << "SUBU   " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x24: { // AND
                ss << "AND    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x25: { // OR
                ss << "OR     " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x26: { // XOR
                ss << "XOR    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x27: { // NOR
                ss << "NOR    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x2a: { // SLT
                ss << "SLT    " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            case 0x2b: { // SLTU
                ss << "SLTU   " << regname(rd) << ", " << regname(rs) << ", " << regname(rt);
                comment2(rs, rt);
                break;
            }
            
            default:    ss << "UNKNOWN SPECIAL " << std::hex << funct;
        }
        
        break;
        
    case 0x01: // REGIMM
        switch (rt) {
            case 0x00: { // BLTZ
                result.target = address + 4 + (static_cast<int32_t>(imm << 2));
                bool taken = static_cast<int32_t>(regs[rs]) < 0;
                ss << "BLTZ   " << regname(rs) << ", " << getFunctionLabel(result.target);
                result.is_branch = true;
                commentBranch(rs, 0, taken); // 0 is dummy, since it's unary
                break;
            }
            
            case 0x01: { // BGEZ
                result.target = address + 4 + (static_cast<int32_t>(imm << 2));
                bool taken = static_cast<int32_t>(regs[rs]) >= 0;
                ss << "BGEZ   " << regname(rs) << ", " << getFunctionLabel(result.target);
                result.is_branch = true;
                commentBranch(rs, 0, taken);
                break;
            }
            
            case 0x10: { // BLTZAL
                result.target = address + 4 + (static_cast<int32_t>(imm << 2));
                bool taken = static_cast<int32_t>(regs[rs]) < 0;
                ss << "BLTZAL " << regname(rs) << ", " << getFunctionLabel(result.target);
                result.is_branch = true;
                commentBranch(rs, 0, taken);
                break;
            }
            
            case 0x11: { // BGEZAL
                result.target = address + 4 + (static_cast<int32_t>(imm << 2));
                bool taken = static_cast<int32_t>(regs[rs]) >= 0;
                ss << "BGEZAL " << regname(rs) << ", " << getFunctionLabel(result.target);
                result.is_branch = true;
                commentBranch(rs, 0, taken);
                break;
            }
        default:
            ss << "UNKNOWN REGIMM " << std::hex << rt;
        }
        break;
        
        case 0x02: { // J
            result.target = ((address + 4) & 0xf0000000) | jump_target;
            ss << "J      " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentJump(result.target);
            break;
        }
        
        case 0x03: { // JAL
            result.target = ((address + 4) & 0xf0000000) | jump_target;
            ss << "JAL    " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentJump(result.target);
            break;
        }
        
        case 0x04: {
            result.target = (address + 4) + (static_cast<int32_t>(imm << 2));
            bool taken = regs[rs] == regs[rt];
            ss << "BEQ    " << regname(rs) << ", " << regname(rt) << ", " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentBranch(rs, rt, taken);
            break;
        }
        
        case 0x05: {
            result.target = (address + 4) + (static_cast<int32_t>(imm << 2));
            bool taken = regs[rs] != regs[rt];
            ss << "BNE    " << regname(rs) << ", " << regname(rt) << ", " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentBranch(rs, rt, taken);
            break;
        }
        
        case 0x06: { // BLEZ
            result.target = address + 4 + (static_cast<int32_t>(imm << 2));
            bool taken = static_cast<int32_t>(regs[rs]) <= 0;
            ss << "BLEZ   " << regname(rs) << ", " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentBranch(rs, 0, taken);
            break;
        }
        
        case 0x07: { // BGTZ
            result.target = address + 4 + (static_cast<int32_t>(imm << 2));
            bool taken = static_cast<int32_t>(regs[rs]) > 0;
            ss << "BGTZ   " << regname(rs) << ", " << getFunctionLabel(result.target);
            result.is_branch = true;
            commentBranch(rs, 0, taken);
            break;
        }
        
    case 0x08: // ADDI
        ss << "ADDI   " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << imm;
    break;
    
    case 0x09: // ADDIU
        ss << "ADDIU  " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << imm;
    break;
    
    case 0x0a: // SLTI
        ss << "SLTI   " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << imm;
    break;
    
    case 0x0b: // SLTIU
        ss << "SLTIU  " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << imm;
    break;
    
    case 0x0c: // ANDI (uses zero-extended immediate)
        ss << "ANDI   " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << uimm;
    break;
    
    case 0x0d: // ORI
        ss << "ORI    " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << uimm;
    break;
    
    case 0x0e: // XORI
        ss << "XORI   " << regname(rt) << ", " << regname(rs) << ", 0x" << std::hex << uimm;
    break;
    
    case 0x0f: ss << "LUI    " << regname(rt) << ", 0x" << std::hex << (uimm << 16); break;
    
    case 0x10: case 0x11: case 0x12: case 0x13: // COP0-3
        ss << "COP" << (inst.func() & 0x03) << "    ";
        switch (inst.copOpcode()) {
        case 0x00: ss << "MFC" << (inst.func() & 0x03) << "  " << regname(rt) << ", $" << rd; break;
        case 0x04: ss << "MTC" << (inst.func() & 0x03) << "  " << regname(rt) << ", $" << rd; break;
        default:   ss << "COP" << (inst.func() & 0x03) << "  " << std::hex << inst.copOpcode();
    }
    
    break;
    
    case 0x20: ss << "LB     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x21: ss << "LH     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x22: ss << "LWL    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x23: ss << "LW     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x24: ss << "LBU    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x25: ss << "LHU    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    case 0x26: ss << "LWR    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm); break;
    
    case 0x28: ss << "SB     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm);break;
    case 0x29: ss << "SH     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm);break;
    case 0x2a: ss << "SWL    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm);break;
    case 0x2b: ss << "SW     " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm);break;
    case 0x2e: ss << "SWR    " << regname(rt) << ", [0x" << std::hex << (regs[rs] + (int16_t)imm) << "]"; comment2(rs, rt); commentMemAccess(rs, imm);break;
    
    case 0x30: case 0x31: case 0x32: case 0x33: // LWC0-LWC3
    case 0x38: case 0x39: case 0x3a: case 0x3b: // SWC0-SWC3
        ss << (inst.func() >= 0x30 && inst.func() <= 0x33 ? "LWC" : "SWC")
           << (inst.func() & 0x03) << "   " << regname(rt)
           << ", 0x" << std::hex << imm << "(" << regname(rs) << ")";
        break;
    
    default:
        ss << "UNKNOWN OPCODE 0x" << std::hex << inst.func();
    }
    
    result.text = ss.str();
    return result;
}

std::string CPU::getFunctionLabel(uint32_t addr) {
    auto& labels = disasmState.functionLabels;
    
    if (labels.count(addr))
        return labels[addr];
    
    std::stringstream ss;
    ss << "func_" << std::hex << addr;
    std::string label = ss.str();
    
    labels[addr] = label;
    disasmState.knownFunctions.insert(addr);
    
    return label;
}

// This is also stolen straight up from
// https://github.com/BluestormDNA/ProjectPSX/blob/master/ProjectPSX/Core/CPU.cs#L128
bool CPU::handleInterrupts(Instruction& instruction) {
    //uint32_t load = interconnect.loadInstruction(pc);
    
    uint32_t instr = instruction.op >> 26;
    
    // Delay instruction
    if(instr == 0x12) {
        // COP2 MTC2
        return false;
    }
    
    bool IEC = (_cop0.sr & 0x1) == 1;
    uint8_t IM = static_cast<uint8_t>(_cop0.sr >> 8) & 0xFF;
    uint8_t IP = static_cast<uint8_t>(_cop0.cause >> 8) & 0xFF;
    
    if(IEC && (IM & IP) > 0) {
        exception(Interrupt);
        
        return true;
    }
    
    return false;
}

void CPU::opsll(Instruction& instruction) {
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
    uint32_t v = reg(t) << i;
    
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
    uint32_t i = instruction.shift();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
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

// Load Upper Immediate(LUI)
void CPU::oplui(Instruction& instruction) {
    auto i = instruction.imm();
    auto t = instruction.t();
    
    auto v = i << 16;
    
    set_reg(t, v);
}

void CPU::opori(Instruction& instruction) {
    uint32_t i = instruction.imm();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t v = reg(s) | i;
    
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
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    // Was doing '!' instead of '~' :)
    uint32_t v = ~(reg(s) | reg(t));
    
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
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    bool v = reg(s) < reg(t);
    
    set_reg(d, static_cast<uint32_t>(v)); // V gets converted into a uint32
}

void CPU::opslti(Instruction& instruction) {
    auto i = static_cast<int32_t>(instruction.imm_se());
    uint32_t s = instruction.s(); 
    uint32_t t = instruction.t();
    
    bool v = (static_cast<int32_t>(reg(s))) < i;
    
    set_reg(t, v);
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
    /*if((sr & 0x10000) != 0) {
        //std::cerr << "Ignoring store-word while cache is isolated!\n";
        
        return;
    }*/
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    if(addr % 4 == 0) {
        uint32_t v    = reg(t);
        
        store32(addr, v);
    } else {
        _cop0.badVaddr = addr;
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
    /*if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-halfword while cache is isolated!\n";
        
        return;
    }*/
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = reg(s) + i;
    
    if(addr % 2 == 0) {
        uint32_t v = reg(t);
        
        store16(addr, v);
    } else {
        _cop0.badVaddr = addr;
       exception(StoreAddressError); 
    }
}

void CPU::opsb(Instruction& instruction) {
    /*if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-byte while cache is isolated!\n";
        
        return;
    }*/
    
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    uint32_t addr = reg(s) + i;
    uint32_t v    = reg(t);
    
    store8(addr, static_cast<uint8_t>(v));
}

// Load word
void CPU::oplw(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto t = instruction.t();
    auto s = instruction.s();
    
    auto addr = reg(s) + i;
    
    if(addr % 4 == 0) {
        uint32_t v = load32(addr);
        
        // Put the load in the delay slot
        setLoad(t, v);
    } else {
        exception(LoadAddressError);
    }
}

void CPU::oplwl(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = reg(s) + i;
    
    // This instruction bypasses the load delay restriction:
    // this instruction will merge the new contents,
    // with the value that is currently being loaded if needed.
    //uint32_t curv = outRegs[static_cast<size_t>(t)];
    //uint32_t curv = rt;
    
    uint32_t rt = t;
    if(loads[0].index == t) {
        rt = loads[0].value;
    } else {
        rt = reg(t);
    }
    
    // Next we load the *aligned* word containing the first address byte
    uint32_t alignedAddr = addr & ~3;
    uint32_t alignedWord = load32(alignedAddr);
    
    // Depending on the address alignment we fetch, 1, 2, 3 or 4,
    // *most* significant bytes and put them in the target register.
    uint32_t v = addr & 3;
    
    switch (v) {
    case 0:
        v = (rt & 0x00FFFFFF) | (alignedWord << 24);
        break;
    case 1:
        v = (rt & 0x0000FFFF) | (alignedWord << 16);
        break;
    case 2:
        v = (rt & 0x000000FF) | (alignedWord << 8);
        break;
    case 3:
        v = (rt & 0x00000000) | (alignedWord << 0);
        break;
    default:
        throw std::runtime_error("Unreachable code!");
    }
    
    // Put the load in the delay slot
    //load = {t, v};
    setLoad(t, v);
}

void CPU::oplwr(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // This instruction bypasses the load delay restriction:
    // this instruction will merge the new contents,
    // with the value that is currently being loaded if needed.
    //uint32_t curv = outRegs[static_cast<size_t>(t)];
    //uint32_t curv = reg(t);
    
    uint32_t rt = t;
    if(loads[0].index == t) {
        rt = loads[0].value;
    } else {
        rt = reg(t);
    }
    
    // Next we load the *aligned* word containing the first address byte
    uint32_t alignedAddr = addr & ~3;
    uint32_t alignedWord = load32(alignedAddr);
    
    // Depending on the address alignment we fetch, 1, 2, 3 or 4,
    // *most* significant bytes and put them in the target register.
    uint32_t v = addr & 3;
    
    switch (v) {
    case 0:
        v = (rt & 0x00000000) | (alignedWord >> 0);
        break;
    case 1:
        v = (rt & 0xFF000000) | (alignedWord >> 8);
        break;
    case 2:
        v = (rt & 0xFFFF0000) | (alignedWord >> 16);
        break;
    case 3:
        v = (rt & 0xFFFFFF00) | (alignedWord >> 24);
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
    /*if((sr & 0x10000) != 0) {
        std::cout << "Ignoring store-word while cache is isolated!\n";
        
        return;
    }*/
    
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
        _cop0.badVaddr = addr;
        exception(LoadAddressError);
    }
}

void CPU::oplhu(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t t = instruction.t();
    uint32_t s = instruction.s();
    
    uint32_t addr = wrappingAdd(reg(s), i);
    
    // Address must be 16bit aligned
    if(addr % 2 == 0) {
        uint16_t v = load16(addr);
        
        setLoad(t, static_cast<uint32_t>(v));
    } else {
        _cop0.badVaddr = addr;
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
    auto i = static_cast<int32_t>(instruction.imm_se());
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
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
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
    uint32_t d = instruction.d();
    
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
        
        gte.write(t, v);
        //_cop2.setData(t, v);
    } else {
        exception(LoadAddressError);
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
        auto v = gte.read(t);//_cop2.getData(t);
        
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
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    uint32_t d = instruction.d();
    
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
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    uint32_t v = reg(s) & reg(t);
    
    set_reg(d, v);
}

void CPU::opandi(Instruction& instruction){
    uint32_t i = instruction.imm();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    uint32_t v = reg(s) & i;
    
    set_reg(t, v);
}

int CPU::opcop0(Instruction& instruction) {
    // Formation goes as:
    // 0b0100nn where nn is the coprocessor number.
    
    switch (instruction.copOpcode()) {
    case 0b000000:
        opmfc0(instruction);
        return 1;
    case 0b00100:
        opmtc0(instruction);
        return 1;
    case 0b10000:
        oprfe(instruction);
        return 1;
    default:
        printf("");
        //std::cerr << "Unhandled COP0 instruction: " + std::to_string(instruction.copOpcode()) << "\n";
        //throw std::runtime_error("Unhandled COP instruction: " + std::to_string((instruction.copOpcode())));
    }
}

// Doesn't exists
int CPU::opcop1(Instruction& instruction) {
    exception(CoprocessorError);
    //throw std::runtime_error("Unhandled coprocessor 1 instructions\n");
    return 1;
}

int CPU::opcop2(Instruction& instruction) {
    auto opcode = instruction.copOpcode();
    
    if (instruction.op & (1 << 25)) {
        // GTE command
        gte::Command command(instruction.op);
        return gte.command(command);
    }
    
    switch (opcode) {
    case 0b000000:
        // Move from GTE to Data register
        opmfc2(instruction);
        return 1;
    case 0b000010:
        // From GTE to Control register
        opcfc2(instruction);
        return 1;
    case 0b00100:
        // Data Register to GTE
        opmtc2(instruction);
        return 1;
    case 0b00110:
        // Data Register to GTE Control register
        opctc2(instruction);
        return 1;
    default:
        std::cerr << "Unhandled COP2 instruction: " + std::to_string((instruction.copOpcode())) << "\n";
        throw std::runtime_error("Unhandled COP instruction: " + std::to_string((instruction.copOpcode())));
    }
}

// Doesn't exists
int CPU::opcop3(Instruction& instruction) {
    exception(CoprocessorError);
    return 1;
}

void CPU::opmtc0(Instruction& instruction) {
    // This basically makes it so that all the data,
    // goes to the cache instead of the main memory.
    uint32_t cpur = instruction.t();
    uint32_t copr = instruction.d();
    
    uint32_t v = reg(cpur);
    
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
    //Breakpoints registers for the future
    case 3: case 5: case 6: case 7: case 9: case 11:
        if(v != 0) {
            std::cerr << "Unhandled write to cop0r{} " << copr << " val: " << v << "\n";
            //throw std::runtime_error("Unhandled write to cop0r{} " + std::to_string(copr) + " val: " + std::to_string(v) + "\n");
        }
        
        break;
    case 12: {
        bool shouldInterrupt = (_cop0.sr & 0x1) == 1;
        bool cur = (v & 0x1) == 1;
        
        _cop0.sr = v;
        
        uint32_t IM = (v >> 8) & 0x3;
        uint32_t IP = (_cop0.cause >> 8) & 0x3;
        
        if(!shouldInterrupt && cur && (IM & IP) > 0) {
            pc = nextpc;
            currentpc = pc;
            //nextpc += 4;
            
            exception(Exception::Interrupt);
        }
        
        break;
    }
    case 13: {
        _cop0.cause &= ~0x300;
        _cop0.cause |= v & 0x300;
        
        break;
    }
    default:
        std::cout << "Unhandled cop0 register " << copr << "\n";
        throw std::runtime_error("Unhandled cop0 register " + std::to_string(copr));
    }
}

void CPU::opmfc0(Instruction& instruction) {
    auto cpur = instruction.t();
    uint32_t copr = instruction.d();
    
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
    // Unknown registers
    case 0:
    case 1:
    case 2:
        break;
    case 6:
    case 7:
        break;
    case 8:
        v = _cop0.badVaddr;
        break;
    case 9:
        v = 0;
        
        break;
    case 12:
        v = _cop0.sr;
        break;
    case 13:
        v = _cop0.cause;
        break;
    case 14:
        v = _cop0.epc;
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
    
    uint32_t mode = _cop0.sr & 0x3F;
    _cop0.sr &= ~0xF;
    _cop0.sr |= mode >> 2;
}

void CPU::opmfc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = gte.read(d);//_cop2.getData(d);
    
    setLoad(t, v);
}

void CPU::opcfc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = gte.read(d + 32);//_cop2.getControl(d);
    
    setLoad(t, v);
}

void CPU::opmtc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = reg(t);
    
    gte.write(d, v);
    //_cop2.setData(d, v);
}

void CPU::opctc2(Instruction& instruction) {
    auto t = instruction.t();
    auto d = instruction.d();
    
    auto v = reg(t);
    
    gte.write(d + 32, v);
    //_cop2.setControl(d, v);
}

void CPU::opgte(Instruction& instruction) {
    throw std::runtime_error("Unhandled GTE operator\n");
}

void CPU::exception(const Exception exception) {
    // Only 0x4h and 0x5h updates BadVaddr
    // Bruh..Shouldn't point to 'currentpc'
    // but the addr it'll jump to?
    /*if(exception == LoadAddressError || exception == StoreAddressError) {
        _cop0.badVaddr = currentpc;
    }*/
    
    // Shift bits [5:0] of 'SR' two places to the left
    uint32_t mode = _cop0.sr & 0x3f;
    
    _cop0.sr &= ~0x3f;
    _cop0.sr |= (mode << 2) & 0x3f;
    
    uint8_t IP = static_cast<uint8_t>(_cop0.cause >> 8) & 0xFF;
    _cop0.cause = 0;
    _cop0.cause |= (IP) << 8;
    
    // Update 'CAUSE' register with the exception code (bits [6:2])
    _cop0.cause &= ~0x7C;
    _cop0.cause |= static_cast<uint32_t>(exception) << 2;
    
    /**
     * Been stuck at this issue for a while so I'm cheating:
     * Avocado seems to set this bit this way, docs say:
     *  
     * 28-29 CE Contains the coprocessor number if the exception
     *          occurred because of a coprocessor instuction for
     *          a coprocessor which wasn't enabled in SR.
     */
    Instruction load = load32(currentpc);
    uint8_t coprocessorNumber = load.func() & 0x3;
    
    _cop0.cause &= ~(0x3 << 28);
    _cop0.cause |= (coprocessorNumber << 28);
    
    if(delayJumpSlot) {
        _cop0.cause |= (static_cast<uint32_t>(1) << (30));
    }
    
    if(delaySlot) {
        // When an exception occurs in a delay slot 'EPC' points
        // to the branch instruction and bit 31 of 'CAUSE' is set.
        _cop0.epc = currentpc - 4;
        _cop0.cause |= (static_cast<uint32_t>(1) << (31));
    } else {
        // Save current instruction address in 'EPC'
        _cop0.epc = currentpc;
        _cop0.cause &= ~(static_cast<uint32_t>(1) << 31);
    }
    
    // Determine the exception handler address based on the 'BEV' bit
    uint32_t handler = (_cop0.sr & (1 << 22)) != 0 ? 0xbfc00180 : 0x80000080;
    
    // Exceptions don’t have a branch delay, we jump directly into the handler
    pc = handler;
    nextpc = handler + 4;
    
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
    paused = true;
    printf("Illegal instruction %d %d PC; %d\n", instruction.func(), instruction.subfunction(), currentpc);
    std::cerr << "Illegal instruction " << instruction.op << " PC; " << currentpc << "\n";
    exception(IllegalInstruction);
}

// Take from; https://github.com/allkern/psxe/blob/master/psx/cpu_debug.h#L3
static const char* g_psx_cpu_a_kcall_symtable[] = {
    "open(filename=%08x,accessmode=%08x)",
    "lseek(fd=%08x,offset=%08x,seektype=%08x)",
    "read(fd=%08x,dst=%08x,length=%08x)",
    "write(fd=%08x,src=%08x,length=%08x)",
    "close(fd=%08x)",
    "ioctl(fd=%08x,cmd=%08x,arg=%08x)",
    "exit(exitcode=%08x)",
    "isatty(fd=%08x)",
    "getc(fd=%08x)",
    "putc(char=%08x,fd=%08x)",
    "todigit(char=%08x)",
    "atof(src=%08x)",
    "strtoul(src=%08x,src_end=%08x,base=%08x)",
    "strtol(src=%08x,src_end=%08x,base=%08x)",
    "abs(val=%08x)",
    "labs(val=%08x)",
    "atoi(src=%08x)",
    "atol(src=%08x)",
    "atob(src=%08x,num_dst=%08x)",
    "setjmp(buf=%08x)",
    "longjmp(buf=%08x,param=%08x)",
    "strcat(dst=%08x,src=%08x)",
    "strncat(dst=%08x,src=%08x,maxlen=%08x)",
    "strcmp(str1=%08x,str2=%08x)",
    "strncmp(str1=%08x,str2=%08x,maxlen=%08x)",
    "strcpy(dst=%08x,src=%08x)",
    "strncpy(dst=%08x,src=%08x,maxlen=%08x)",
    "strlen(src=%08x)",
    "index(src=%08x,char=%08x)",
    "rindex(src=%08x,char=%08x)",
    "strchr(src=%08x,char=%08x)",
    "strrchr(src=%08x,char=%08x)",
    "strpbrk(src=%08x,list=%08x)",
    "strspn(src=%08x,list=%08x)",
    "strcspn(src=%08x,list=%08x)",
    "strtok(src=%08x,list=%08x)",
    "strstr(str=%08x,substr=%08x)",
    "toupper(char=%08x)",
    "tolower(char=%08x)",
    "bcopy(src=%08x,dst=%08x,len=%08x)",
    "bzero(dst=%08x,len=%08x)",
    "bcmp(ptr1=%08x,ptr2=%08x,len=%08x)",
    "memcpy(dst=%08x,src=%08x,len=%08x)",
    "memset(dst=%08x,fillbyte=%08x,len=%08x)",
    "memmove(dst=%08x,src=%08x,len=%08x)",
    "memcmp(src1=%08x,src2=%08x,len=%08x)",
    "memchr(src=%08x,scanbyte=%08x,len=%08x)",
    "rand()",
    "srand(seed=%08x)",
    "qsort(base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "strtod(src=%08x,src_end=%08x)",
    "malloc(size=%08x)",
    "free(buf=%08x)",
    "lsearch(key=%08x,base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "bsearch(key=%08x,base=%08x,nel=%08x,width=%08x,callback=%08x)",
    "calloc(sizx=%08x,sizy=%08x)",
    "realloc(old_buf=%08x,new_siz=%08x)",
    "InitHeap(addr=%08x,size=%08x)",
    "_exit(exitcode=%08x)",
    "getchar()",
    "putchar(char=%08x)",
    "gets(dst=%08x)",
    "puts(src=%08x)",
    "printf(txt=%08x,param1=%08x,param2=%08x,etc.=%08x)",
    "SystemErrorUnresolvedException()",
    "LoadTest(filename=%08x,headerbuf=%08x)",
    "Load(filename=%08x,headerbuf=%08x)",
    "Exec(headerbuf=%08x,param1=%08x,param2=%08x)",
    "FlushCache()",
    "init_a0_b0_c0_vectors()",
    "GPU_dw(Xdst=%08x,Ydst=%08x,Xsiz=%08x,Ysiz=%08x,src=%08x)",
    "gpu_send_dma(Xdst=%08x,Ydst=%08x,Xsiz=%08x,Ysiz=%08x,src=%08x)",
    "SendGP1Command(gp1cmd=%08x)",
    "GPU_cw(gp0cmd=%08x)",
    "GPU_cwp(src=%08x,num=%08x)",
    "send_gpu_linked_list(src=%08x)",
    "gpu_abort_dma()",
    "GetGPUStatus()",
    "gpu_sync()",
    "SystemError()",
    "SystemError()",
    "LoadExec(filename=%08x,stackbase=%08x,stackoffset=%08x)",
    "GetSysSp()",
    "SystemError()",
    "_96_init()",
    "_bu_init()",
    "_96_remove()",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "dev_tty_init()",
    "dev_tty_open(fcb=%08x, (unused)path=%08x,accessmode=%08x)",
    "dev_tty_in_out(fcb=%08x,cmd=%08x)",
    "dev_tty_ioctl(fcb=%08x,cmd=%08x,arg=%08x)",
    "dev_cd_open(fcb=%08x,path=%08x,accessmode=%08x)",
    "dev_cd_read(fcb=%08x,dst=%08x,len=%08x)",
    "dev_cd_close(fcb=%08x)",
    "dev_cd_firstfile(fcb=%08x,path=%08x,direntry=%08x)",
    "dev_cd_nextfile(fcb=%08x,direntry=%08x)",
    "dev_cd_chdir(fcb=%08x,path=%08x)",
    "dev_card_open(fcb=%08x,path=%08x,accessmode=%08x)",
    "dev_card_read(fcb=%08x,dst=%08x,len=%08x)",
    "dev_card_write(fcb=%08x,src=%08x,len=%08x)",
    "dev_card_close(fcb=%08x)",
    "dev_card_firstfile(fcb=%08x,path=%08x,direntry=%08x)",
    "dev_card_nextfile(fcb=%08x,direntry=%08x)",
    "dev_card_erase(fcb=%08x,path=%08x)",
    "dev_card_undelete(fcb=%08x,path=%08x)",
    "dev_card_format(fcb=%08x)",
    "dev_card_rename(fcb1=%08x,path=%08x)",
    "card_clear_error(fcb=%08x) (?)",
    "_bu_init()",
    "_96_init()",
    "_96_remove()",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "CdAsyncSeekL(src=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "CdAsyncGetStatus(dst=%08x)",
    "return 0",
    "CdAsyncReadSector(count=%08x,dst=%08x,mode=%08x)",
    "return 0",
    "return 0",
    "CdAsyncSetMode(mode=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "CdromIoIrqFunc1()",
    "CdromDmaIrqFunc1()",
    "CdromIoIrqFunc2()",
    "CdromDmaIrqFunc2()",
    "CdromGetInt5errCode(dst1=%08x,dst2=%08x)",
    "CdInitSubFunc()",
    "AddCDROMDevice()",
    "AddMemCardDevice()",
    "AddDuartTtyDevice()",
    "add_nullcon_driver()",
    "SystemError()",
    "SystemError()",
    "SetConf(num_EvCB=%08x,num_TCB=%08x,stacktop=%08x)",
    "GetConf(num_EvCB_dst=%08x,num_TCB_dst=%08x,stacktop_dst=%08x)",
    "SetCdromIrqAutoAbort(type=%08x,flag=%08x)",
    "SetMem(megabytes=%08x)",
    "_boot()",
    "SystemError(type=%08x,errorcode=%08x)",
    "EnqueueCdIntr()",
    "DequeueCdIntr()",
    "CdGetLbn(filename=%08x)",
    "CdReadSector(count=%08x,sector=%08x,buffer=%08x)",
    "CdGetStatus()",
    "bufs_cb_0()",
    "bufs_cb_1()",
    "bufs_cb_2()",
    "bufs_cb_3()",
    "_card_info(port=%08x)",
    "_card_load(port=%08x)",
    "_card_auto(flag=%08x)",
    "bufs_cb_4()",
    "card_write_test(port=%08x)",
    "return 0",
    "return 0",
    "ioabort_raw(param=%08x)",
    "return 0",
    "GetSystemInfo(index=%08x)"
};

static const char* g_psx_cpu_b_kcall_symtable[] = {
    "alloc_kernel_memory(size=%08x)",
    "free_kernel_memory(buf=%08x)",
    "init_timer(t=%08x,reload=%08x,flags=%08x)",
    "get_timer(t=%08x)",
    "enable_timer_irq(t=%08x)",
    "disable_timer_irq(t=%08x)",
    "restart_timer(t=%08x)",
    "DeliverEvent(class=%08x, spec=%08x)",
    "OpenEvent(class=%08x,spec=%08x,mode=%08x,func=%08x)",
    "CloseEvent(event=%08x)",
    "WaitEvent(event=%08x)",
    "TestEvent(event=%08x)",
    "EnableEvent(event=%08x)",
    "DisableEvent(event=%08x)",
    "OpenTh(reg_PC=%08x,reg_SP_FP=%08x,reg_GP=%08x)",
    "CloseTh(handle=%08x)",
    "ChangeTh(handle=%08x)",
    "jump_to_00000000h()",
    "InitPAD2(buf1=%08x,siz1=%08x,buf2=%08x,siz2=%08x)",
    "StartPAD2()",
    "StopPAD2()",
    "PAD_init2(type=%08x,button_dest=%08x,unused=%08x,unused=%08x)",
    "PAD_dr()",
    "ReturnFromException()",
    "ResetEntryInt()",
    "HookEntryInt(addr=%08x)",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "UnDeliverEvent(class=%08x,spec=%08x)",
    "SystemError()",
    "SystemError()",
    "SystemError()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "SystemError()",
    "SystemError()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "jump_to_00000000h()",
    "open(filename=%08x,accessmode=%08x)",
    "lseek(fd=%08x,offset=%08x,seektype=%08x)",
    "read(fd=%08x,dst=%08x,length=%08x)",
    "write(fd=%08x,src=%08x,length=%08x)",
    "close(fd=%08x)",
    "ioctl(fd=%08x,cmd=%08x,arg=%08x)",
    "exit(exitcode=%08x)",
    "isatty(fd=%08x)",
    "getc(fd=%08x)",
    "putc(char=%08x,fd=%08x)",
    "getchar()",
    "putchar(char=%08x)",
    "gets(dst=%08x)",
    "puts(src=%08x)",
    "cd(name=%08x)",
    "format(devicename=%08x)",
    "firstfile2(filename=%08x,direntry=%08x)",
    "nextfile(direntry=%08x)",
    "rename(old_filename=%08x,new_filename=%08x)",
    "erase(filename=%08x)",
    "undelete(filename=%08x)",
    "AddDrv(device_info=%08x)",
    "DelDrv(device_name_lowercase=%08x)",
    "PrintInstalledDevices()",
    "InitCARD2(pad_enable=%08x)",
    "StartCARD2()",
    "StopCARD2()",
    "_card_info_subfunc(port=%08x)",
    "_card_write(port=%08x,sector=%08x,src=%08x)",
    "_card_read(port=%08x,sector=%08x,dst=%08x)",
    "_new_card()",
    "Krom2RawAdd(shiftjis_code=%08x)",
    "SystemError()",
    "Krom2Offset(shiftjis_code=%08x)",
    "_get_errno()",
    "_get_error(fd=%08x)",
    "GetC0Table()",
    "GetB0Table()",
    "_card_chan()",
    "testdevice(devicename=%08x)",
    "SystemError()",
    "ChangeClearPAD(int=%08x)",
    "_card_status(slot=%08x)",
    "_card_wait(slot=%08x)"
};

static const char* g_psx_cpu_c_kcall_symtable[] = {
    "EnqueueTimerAndVblankIrqs(priority=%08x)",
    "EnqueueSyscallHandler(priority=%08x)",
    "SysEnqIntRP(priority=%08x,struc=%08x)",
    "SysDeqIntRP(priority=%08x,struc=%08x)",
    "get_free_EvCB_slot()",
    "get_free_TCB_slot()",
    "ExceptionHandler()",
    "InstallExceptionHandlers()",
    "SysInitMemory(addr=%08x,size=%08x)",
    "SysInitKernelVariables()",
    "ChangeClearRCnt(t=%08x,flag=%08x)",
    "SystemError()",
    "InitDefInt(priority=%08x)",
    "SetIrqAutoAck(irq=%08x,flag=%08x)",
    "return 0",
    "return 0",
    "return 0",
    "return 0",
    "InstallDevices(ttyflag=%08x)",
    "FlushStdInOutPut()",
    "return 0",
    "_cdevinput(circ=%08x,char=%08x)",
    "_cdevscan()",
    "_circgetc(circ=%08x)",
    "_circputc(char=%08x,circ=%08x)",
    "_ioabort(txt1=%08x,txt2=%08x)",
    "set_card_find_mode(mode=%08x)",
    "KernelRedirect(ttyflag=%08x)",
    "AdjustA0Table()",
    "get_card_find_mode()"
};

void CPU::checkForTTY() {
    uint32_t r9 = regs[9];
    
    auto log_strncmp = [this]() {
        uint32_t str1 = regs[4];  // $a0
        uint32_t str2 = regs[5];  // $a1
        uint32_t len = regs[6];   // $a2
        
        char s1[256] = {0};
        char s2[256] = {0};
        
        uint32_t copy_len = std::min(len, 255u);
        
        for(uint32_t i = 0; i < copy_len; i++) {
            s1[i] = load8(str1 + i);
            s2[i] = load8(str2 + i);
            
            // Stop at null terminator if present
            if(s1[i] == '\0' || s2[i] == '\0') break;
        }
        
        std::cerr << "STRNCMP: '";
        for(uint32_t i = 0; i < copy_len && s1[i] != '\0'; i++) {
            std::cerr << (s1[i] >= 32 && s1[i] <= 126 ? s1[i] : '.');
        }
        
        std::cerr << "' vs '";
        for(uint32_t i = 0; i < copy_len && s2[i] != '\0'; i++) {
            std::cerr << (s2[i] >= 32 && s2[i] <= 126 ? s2[i] : '.');
        }
        
        std::cerr << "' (len: " << len << ")\n";
    };
    
    // Bios functions - Tests
    if(pc == 0x000000A0) {
        if(r9 < std::size(g_psx_cpu_a_kcall_symtable)) {
            auto function = g_psx_cpu_a_kcall_symtable[r9];
            
            if(r9 == 24) {  // strncmp is at index 23
                log_strncmp();
            } else {
                std::cerr << function << "\n";
            }
        } else {
            printf("");
        }
    }
    
    if(pc == 0x000000B0) {
        if(r9 < std::size(g_psx_cpu_b_kcall_symtable) && r9 != 44) {
            auto function = g_psx_cpu_b_kcall_symtable[r9];
            
            std::cerr << function << "\n";
        } else {
            printf("");
        }
    }
    
    if(pc == 0x000000C0) {
        if(r9 < std::size(g_psx_cpu_c_kcall_symtable)) {
            auto function = g_psx_cpu_c_kcall_symtable[r9];
            
            std::cerr << function << "\n";
        } else {
            printf("");
        }
    }
    
    /*if ((pc == 0x000000A0 || pc == 0x000000B0 || pc == 0x000000C0)) {
        //if (r9 == 50/*r9 == 0x0000003C || r9 == 0x0000003D#2#) {
            char ch = static_cast<char>(reg(4) & 0xFF);
            
            if ((ch >= 32 && ch <= 126) || ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ') {
                std::cerr << ch;
            }
        //}
    }*/
}

void CPU::reset() {
    branchSlot = false;
    jumpSlot = false;

    delaySlot = false;
    delayJumpSlot = false;
    
    currentpc = 0;
    pc = 0xBFC00000;
    nextpc = pc + 4;
    
    hi = lo = 0;
    
    loads[0] = {};
    loads[1] = {};
    
    for(int i = 0; i < 32; i++)
        regs[i] = 0;
    
    interconnect.reset();
    _cop0.reset();
    gte.reset();
}

void CPU::opj(Instruction& instruction) {
    if(pc == 0x80031dbc)
        return;
    
    uint32_t i = instruction.imm_jump();
    
    nextpc = (pc & 0xf0000000) | (i << 2);
    
    branchSlot = true;
    jumpSlot = true;
}

void CPU::opjr(Instruction& instruction) {
    uint32_t s = instruction.s();
    uint32_t addr = reg(s);
    
    branchSlot = true;
    
    if(addr % 4 != 0) {
        _cop0.badVaddr = addr;
        exception(LoadAddressError);
        
        return;
    }
    
    nextpc = addr;
    jumpSlot = true;
}

void CPU::opjal(Instruction& instruction) {
    // Store return address in £31($ra)
    set_reg(31, nextpc);
    
    opj(instruction);
}

void CPU::opjalr(Instruction& instruction) {
    uint32_t d = instruction.d();
    uint32_t s = instruction.s();
    
    uint32_t addr = reg(s);
    
    branchSlot = true;
    
    set_reg(d, nextpc);
    
    /**
     * TODO;
     * 
     * jalr value error @ 0,2: got ffffffff wanted 001fc010
     * jalr value error @ 3,2: got ffffffff wanted 801fc094
     * jalr value error @ 6,2: got ffffffff wanted a01fc118
     * jalr exception error @ 9: got 00000000 wanted 00000004
     * jalr exception error @ 12: got 00000000 wanted 00000004
     * jalr exception error @ 15: got 00000000 wanted 00000004
     */
    
    if(addr % 4 != 0) {
        _cop0.badVaddr = addr;
        exception(LoadAddressError);
        
        return;
    }
    
    nextpc = addr;
    jumpSlot = true;
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
        int32_t offset = (int32_t((pc) + (i * 4)));
        
        branch(offset);
    }
    
    branchSlot = true;
}

void CPU::opbne(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    // Check if not equal
    if(reg(s) != reg(t)) {
        // Jump to the offset of i
        int32_t offset = (int32_t((pc) + (i * 4)));
        
        branch(offset);
    }
    
    branchSlot = true;
}

void CPU::opbeq(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t s = instruction.s();
    uint32_t t = instruction.t();
    
    // 0x80031614 -> Checks for GPU bit 14, just always never take it I suppose
    // 0x80031624 -> Checks for another bit from the GPU
    
    /**
     * Some function:
     * 80031604: LW     v1(3), [0x1f801814]    ; s2(18)=0x1f800000, v1(3)=0x1c4e0000    ; accessing GPU + 0x4
     * 80031608: LW     v0(2), [0x80044e98]    ; s1(17)=0x80040000, v0(2)=0x00000000    ; accessing RAM + 0x44e98
     * 8003160C: SLL    zero(0), zero(0), 0
     * 80031610: BEQ    v0(2), zero(0), func_80031694    ; v0(2)=0x00000000, zero(0)=0x00000000 ? TAKEN
     * 80031614: ANDI   a0(4), v1(3), 0x2000
     * 80031618: BLTZ   v1(3), func_800316a4    ; v1(3)=0x1c4e0000, zero(0)=0x00000000 ? NOT TAKEN
     * 8003161C: SLL    zero(0), zero(0), 0
     * 80031620: BEQ    a0(4), zero(0), func_800316a4    ; a0(4)=0x00000000, zero(0)=0x00000000 ? TAKEN
     * 80031624: SLTIU  v0(2), v0(2), 0x1
     */
    
    // CPU stuck here?
    if(pc == 0x80031614) {
        printf("");
    }
    
    // 0x801db49c - Checks something from the GPU's bit 24?
    
    //800316AC: BEQ    zero(0), zero(0), func_80031604    ; zero(0)=0x00000000, zero(0)=0x00000000 ? TAKEN
    if(reg(s) == reg(t) /*&& pc != 0x800316b0*/ /*|| pc == 0x801db49c*/) {
        int32_t offset = (int32_t((pc) + (i * 4)));
        
        branch(offset);
    }
    
    branchSlot = true;
}

void CPU::opbqtz(Instruction& instruction) {
    auto i = instruction.imm_se();
    uint32_t s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v > 0) {
        int32_t offset = (int32_t((pc) + (i * 4)));
        
        branch(offset);
    } else {
        printf("");
    }
    
    branchSlot = true;
}

// TODO; Rename to blez(branch on less than or equal to zero)
void CPU::opbltz(Instruction& instruction) {
    auto i = instruction.imm_se();
    auto s = instruction.s();
    
    int32_t v = static_cast<int32_t>(reg(s));
    
    if(v <= 0) {
        int32_t offset = (int32_t((pc) + (i * 4)));
        
        branch(offset);
    }
    
    branchSlot = true;
}

void CPU::branch(uint32_t offset) {
    // Offset immediate are always shifted to two places,
    // to the right as 'PC' addresses have to be aligned with 32 bits.
    //offset = offset << 2;
    
    nextpc = offset;//pc + (offset * 4);
    
    branchSlot = true;
    jumpSlot = true;
}

void CPU::add(Instruction& instruction) {
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
    // Check if cache is isolated
    //assert((sr & 0x10000) == 0x10000);
    
    return interconnect.load<uint32_t>(addr);
}

uint16_t CPU::load16(uint32_t addr) {
    // Check if cache is isolated
    //assert((sr & 0x10000) == 0x10000);
    
    return interconnect.load<uint16_t>(addr);
}

uint8_t CPU::load8(uint32_t addr) {
    // Check if cache is isolated
    //assert((sr & 0x10000) == 0x10000);
    
    return interconnect.load<uint8_t>(addr);
}

void CPU::store32(uint32_t addr, uint32_t val) {
    // Check if cache is isolated
    if((_cop0.sr & 0x10000) != 0) {
        handleCache(addr, val);
    } else {
        interconnect.store<uint32_t>(addr, val);
    }
}

void CPU::store16(uint32_t addr, uint16_t val) {
    // Check if cache is isolated
    //assert((sr & 0x10000) == 0x10000);
    
    if((_cop0.sr & 0x10000) != 0) {
        handleCache(addr, val);
    } else {
        interconnect.store<uint16_t>(addr, val);
    }
}

void CPU::store8(uint32_t addr, uint8_t val) {
    // Check if cache is isolated
    //assert((sr & 0x10000) == 0);
    
    if((_cop0.sr & 0x10000) != 0) {
        handleCache(addr, val);
    } else {
        interconnect.store<uint8_t>(addr, val);
    }
}

void CPU::handleCache(uint32_t addr, uint32_t val) {
    auto cc = interconnect._cacheControl;
    
    if(!cc.isCacheEnabled()) {
        printf("Bruh??? This shouldn't happen :(");
        std::cerr << "";
        return;
    }
    
    uint32_t tag = (addr & 0xFFFFF000) >> 12;
    uint16_t index = (addr & 0xFFC) >> 2;
    interconnect.icache[index] = ICache(tag, val);
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
