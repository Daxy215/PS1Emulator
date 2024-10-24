// Core.cpp

#include <iostream>
#include <fstream>

#include <SDL.h>

#include "Emulator/Bios.h"
#include "Emulator/CPU/CPU.h"
#include "Emulator/SPU/SPU.h"
#include "Utility/FileSystem/FileManager.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//#include "Emulator/interconnect.h"
//#include "Emulator/Ram.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ! Please note most of the stuff here isn't really written by me,    !
// ! I mean the comments, of course.. I am only using it as a reminder !
// ! or a quick lookup table or of sorts.                              !
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// References
// https://github.com/deadcore/playstation-emulator/blob/master/src/cpu/mod.rs
// https://www.reddit.com/r/EmuDev/comments/hvb042/how_to_write_an_ps1_emulator/
// The most savour of ALL https://problemkaputt.de/psx-spx.htm#cpuspecifications
// https://en.wikipedia.org/wiki/NOP // For null functions etc..
// https://en.wikipedia.org/wiki/R3000
// https://www.dsi.unive.it/~gasparetto/materials/MIPS_Instruction_Set.pdf
// https://github.com/Lameguy64/PSn00bSDK

// Used this A LOT to figure out which instruction I need to translate the instructions further
// https://inst.eecs.berkeley.edu/~cs61c/resources/MIPS_help.html
// R-TYPE INSTRUCTIONS -> sub functions
// J-TYPE INSTRUCTIONS -> ??? Jump instructions? COP??
// I-TYPE INSTRUCTIONS -> functions
//
// https://en.wikipedia.org/wiki/MIPS_architecture
// https://www.dsi.unive.it/~gasparetto/materials/MIPS_Instruction_Set.pdf
//
// Explains the process VERY well
// https://psx.arthus.net/docs/MIPS%20Instruction%20Set-harvard.pdf
// https://app.box.com/s/q1tl8yuufsftosvxqyce
// https://app.box.com/s/lmr4nw30cvdhdk5ng7ex
// https://www.cs.columbia.edu/~sedwards/classes/2012/3827-spring/mips-isa.pdf
// https://gist.github.com/dbousamra/f662f381d33fcf5c4a5475c4a656fa19
//
// Helped a lot with the GPU
// https://psx-spx.consoledev.net/graphicsprocessingunitgpu/
// https://github.com/psx-spx/psx-spx.github.io/blob/master/docs/interrupts.md
// https://github.com/allkern/cdrom/blob/master/cdrom.h
// https://s3-eu-west-1.amazonaws.com/downloads-mips/documents/MD00086-2B-MIPS32BIS-AFP-05.04.pdf

/* List of problems;
 * Wrapped_add and wrapped_sub were returning the wrong values,
    * in the end, I learnt that the default c++ behaviour,
    * handles this problem, so I just returned a simple sum of the two
    
 * wrong pc counting.. MANY.. MANY different times
    * (Future me here; Had even more PC problems) 
    * This also caused the program to read the wrong instructions
    * and skip some..
    
 * checked_add -> caused a problem of returning overflow when it shouldn't
 * instruction -> imm_see returns the wrong value
    * Can't remember exactly why, but after a bunch of debugging,
    * I found out that I had a bug somewhere else
     
 * sub instruction -> Was using wrappingadd instead of wrappingsub..
 */

/**
 * Where I got the BIOS from:
 * https://myrient.erista.me/files/Redump/Sony%20-%20PlayStation%20-%20BIOS%20Images/
 * 
 * Needed the ps-22a which is the SCPH-1001 version.
 */

/** IMPORTANT INFORMATION
 * PS1 uses the MIPS instruction set.
 * Each instruction is exactly 32 bits long or 4 bytes.
 * 
 * Initial value of PC is 0xbfc00000(The beginning of the BIOS)
 * 
 * ONLY one DEVICE can access the BUS at a time! (TODO; Research why future me)
 * The DMA can only copy data between the RAM and a device!!
 * 
 * COPIED!!
 * Implementing complete and accurate DMA support can be quite tricky. The
 * main problem is that in certain modes the DMA sporadically gives back the
 * control to the CPU. For instance while the GPU is busy processing a command
 * and won’t accept any new input the DMA has to wait. Instead of wasting time
 * it gives back control to the CPU to give it the opportunity to do something else.
 * In order to emulate this behaviour correctly we need to emulate the GPU
 * command FIFO, DMA timings and CPU timings correctly. Then we need to
 * setup the state machine to switch between the CPU and DMA when needed.
 * That would require quite some work to get right and we only have the BIOS
 * boot logo to test it at this point.
 * To avoid having to implement all that we’re going to make a simplifying
 * assumption for now: when the DMA runs it does all the transfer at once without
 * giving back control to the CPU. This won’t be exactly accurate but it should
 * suffice to run the BIOS and hopefully some games.
 * The reason I feel confident doing this simplification is that PCSX-R seems to
 * do it that way and it can run quite many games, although some comments hint
 * that it breaks with certain titles and it uses some hacks to improve compatibility.
 * Mednafen on the other hand implements a much accurate DMA and actually
 * emulates the DMA giving back the control to the CPU in certain situations,
 * we’ll probably want to do something similar later on.
 * For now let’s take a few steps back and revisit all the DMA register reads
 * and writes done by the BIOS so that we can emulate them correctly.
 * 
 * https://psx-spx.consoledev.net/dmachannels/#dma-register-summary
 * 
 * There 7 DMA channels:
    * Channel 0 -> Is used to connect to the Media Decoder input
    * Channel 1 -> Is used to connect to the Media Decoder output
    * Channel 2 -> Is used to connect to the GPU
    * Channel 3 -> Is used to connect to the CDROM drive
    * Channel 4 -> Is used to connect to the SPU
    * Channel 5 -> Is used to connect to the extension port
    * Channel 6 -> Is used to connect to the RAM and is used to clear an "ordering table"
    
 * DMA Channel Control register description
    * Field bits        Description
    * 0                 Transfer direction: RAM-to-device(0) or device-RAM(1)
    * 1                 Address increment(0) or decrement(1) mode
    * 2                 Chopping mode
    * [10:9]            Synchronization type: Manual(0), Request(1) or Linked List(2)
    * [18:16]           Chopping DMA window
    * [22:20]           Chopping CPU window
    * 24                Enable
    * 28                Manual trigger
    * [30:29]           Unknown
    * 
    * PAGE 28 for more information
    
 * The PS1 uses little-endian formation for storing data.
    * As they store the least significant byte first.
    
 * MIPS R3000 CPU can support up to 4 coprocessors:
    * Coprocessor 0 (cop0):
        * Is basically used for exception handling.
        
    * Coprocessor 1 (cop1):
        * Is an optional coprocessor that is available for floating points arithmetic usage.
        
    * Coprocessor 2 (cop2):
        * Seems to be custom-made for the Playstation? However, it's called,
        * Geometry Transformation Engine (GTE). Used for... 3D stuff, such as,
        * perspectives, transformations, matrices etc..
         
    * Coprocessor 3 (cop3):
        * This one isn't really implemented on the Playstation.
 */

/** More registers information
 * $cop0 3 is BPC, used to generate a breakpoint exception when,
    * the PC takes the given value.
    
 * $cop0 5 is BDA, the data breakpoint. It’s like BPC except it breaks when
    * a certain address is accessed on a data load/store instead of a PC value.
    
 * $cop0 6: No information was found regarding this register
    
 * $cop0 7 is DCIC, used to enable and disable the various hardware breakpoints.
 * $cop0 9 is BDAM, it’s a bitmask applied when testing for BDA above.
    * That way we could trigger on a range of addresses instead of a single one.
    
 * $cop0 11 is BPCM, like BDAM but for masking the BPC breakpoint.
 * $cop0 12 we’ve already encountered: it’s SR, the status register.
 * $cop0 13 is CAUSE, which contains mostly read-only data describing the
    * cause of an exception. Apparently, only bits [9:8] are writable to force an exception.
 */

// Tables; THEY AREN'T MADE BY ME!  ¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?
// I mostly got them through the guide.

// PS1 Memory Map
// KUSEG        KSEG0        KSEG1        Length  Description
// 0x00000000   0x80000000   0xa0000000   2048K   Main RAM
// 0x1f000000   0x9f000000   0xbf000000   8192K   Expansion Region 1
// 0x1f800000   0x9f800000   0xbf800000   1K      Scratchpad
// 0x1f801000   0x9f801000   0xbf801000   8K      Hardware registers
// 0x1fc00000   0x9fc00000   0xbfc00000   512K    BIOS ROM
// Reads from bottom up(I think?)

// KSEG2        Length      Description
// 0xfffe0000   512B        I/O Ports

/** Without the coprocessors(for now.) They all are 32 bits.
 * 
 * Register            Name              Conventional use
 * $0                  $zero             Always zero
 * $1                  $at               Assembler temporary
 * $2, $3              $v0, $v1          Function return values
 * $4 . . . $7         $a0 . . . $a3     Function arguments
 * $8 . . . $15        $t0 . . . $t7     Temporary registers
 * $16 . . . $23       $s0 . . . $s7     Saved registers
 * $24, $25            $t8, $t9          Temporary registers
 * $26, $27            $k0, $k1          Kernel reserved registers
 * $28                 $gp               Global pointer
 * $29                 $sp               Stack pointer
 * $30                 $fp               Frame pointer
 * $31                 $ra               Function return address
 */

/** 16 to 32bits conversion: influence of sign extension
 *  16 bit value        32bit "unsigned" extended value         decimal unsigned value
 *  0x0000              0x00000000                              0
 *  0x0001              0x00000001                              1
 *  0x01ad              0x000001ad                              429
 *  0xffff              0x0000ffff                              65535
 *  0x83c5              0x000083c5                              33733
 * 
 * 16 bit value        32bit sign-extended value                decimal unsigned value
 *  0x0000              0x00000000                              0
 *  0x0001              0x00000001                              1
 *  0x01ad              0x000001ad                              429
 *  0xffff              0xffffffff                              -1
 *  0x83c5              0xffff83c5                              -31803
 */

/** Registers table
 * $zero (Register 0): Constant value 0
 * $at (Register 1): Assembler temporary
 * $v0 - $v1 (Registers 2-3): Function result
 * $a0 - $a3 (Registers 4-7): Function arguments
 * $t0 - $t7 (Registers 8-15): Temporary registers
 * $s0 - $s7 (Registers 16-23): Saved registers
 * $t8 - $t9 (Registers 24-25): Temporary registers
 * $k0 - $k1 (Registers 26-27): Reserved for OS kernel
 * $gp (Register 28): Global pointer
 * $sp (Register 29): Stack pointer
 * $fp (Register 30): Frame pointer
 * $ra (Register 31): Return address
 */

/** Information about some registers;
 *
 * Please note this is NOT written by me.
 * It is simply used as a further guide to myself.
 *
 * $zero register;
    * ($0) is ALWAYS equal to 0;
        * If an instruction attempts to load a value in this register,
        * it doesn't do anything, the register will still be 0 afterwards.
            * It can be used to reduce the size of the instruction as it's a constant 0.
        * For instance, moving a value from $v0 to $a0 you can do;
            * move $a0, $v0
        * However the "move" instruction is not part of the MIPS instruction set.
        * It's just a conveninet shorthand understood by the assembler which,
        * which will generate the equivalent instruction;
            * addu $a0, $v0, $zero
        * We can see that it is effectively does the same thing as the previous instruction,
        * but we are avoid to implement a dedicated "move" instruction in the CPU.
        
 * The $ra register;
    * $ra ($31) is the other general purpose register given a special meaning by the hardware,
    * since instructions like "jump and link" or "branch and link" put the return address,
    * in this register exclusively. Therefore, the following instruction,
    * jumps in a function foo and puts the return address in $ra;
        * jal foo
 */

/*
 * Name     Description
 * PC       Program counter
 * HI       high 32bits of multiplication result; remainder of divion
 * LO       low 32bits of multiplication result; quotient of divion
 */

// Other thingies...... ¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?

/** Gotta do more research on
 *
 * File type??? 001111 [31:26]
 * LUI(Load Upper Immediate)
 *
 * Information about it;
    * Immediate means that the value loaded is directly in the instruction,
    * not indirectly somewhere else in memory.
    *
    * Upper means that it's loading this immediate value into the high 16 bits,
    * of the target register.
    *
    * The 16 low bits are cleared (set to 0).
    *
    * Which is equivalent to MIPS assembly:
        * lui $8, 0x13
    
    * Dont get ANY of this. Page 16
 */

struct Exe {
	char header[8];
	
	uint32_t text;
	uint32_t data;
	
	uint32_t pc0;
	uint32_t gp0;
	
	uint32_t tAddr;
	uint32_t tSize;
	
	// Both of those are unnkown
	uint32_t dAddr;
	uint32_t dSize;
	
	uint32_t bAddr;
	uint32_t bSize;
	
	uint32_t sAddr;
	uint32_t sSize;
	
	uint32_t sp, fp, gp, ret, base;
	
	char license[60];
};

int main(int argc, char* argv[]) {
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
    Ram ram;
    Bios bios = Bios("BIOS/ps-22a.bin");
    Dma dma;
	
    // TODO; Texture loading
    Emulator::Gpu gpu;
    
    // TODO; Implement
    Emulator::SPU spu = Emulator::SPU();
    
    CPU cpu = CPU(Interconnect(ram, bios, dma, gpu, spu));
	
    SDL_Event event;
	
    int x = 0;
	uint32_t counter = 0;
	
    while(true) {
    	if(x == 129922852) {
    		printf("");
    	}
    	
    	// TODO; Testing
    	if(counter++ > 2560000) {
    		counter = 0;
    		
    		cpu.interconnect._joypad.update();
			
    		// TODO; Remove
    		gpu.renderer->display();
    	}
		
    	bool enableBios = true;
    	
	    // Wait for the BIOS to jump to the shell
	    if (cpu.pc != 0x80030000 || enableBios) {
		    cpu.executeNextInstruction();
	    } else {
	    	std::cerr << "Loading test EXE file\n";
	    	
	    	// Tests
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ADD/CPUADD.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ADDI/CPUADDI.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ADDIU/CPUADDIU.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ADDU/CPUADDU.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/AND/CPUAND.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ANDI/CPUANDI.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/DIV/CPUDIV.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/DIVU/CPUDIVU.exe"); // Passed
			
	    	// Load Tests
	    	//td::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/LB/CPULB.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/LH/CPULH.exe"); // Passed - This was failing bc of LH. V was meant to be in16 but it was uint32
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/LW/CPULW.exe"); // Passed
			
	    	// Store Tests
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/SB/CPUSB.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/SH/CPUSH.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/SW/CPUSW.exe"); // Passed
	    	
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/MULT/CPUMULT.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/MULTU/CPUMULTU.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/NOR/CPUNOR.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/OR/CPUOR.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/ORI/CPUORI.exe"); // Passed
			
	    	// Shift tests
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/Sll/CPUSlL.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/SllV/CPUSlLV.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/SRA/CPUSRA.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/SRAV/CPUSRAV.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/SRL/CPUSRL.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SHIFT/SRLV/CPUSRLV.exe"); // Passed
			
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SUB/CPUSUB.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/SUBU/CPUSUBU.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/XOR/CPUXOR.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/XORI/CPUXORI.exe"); // Passed
	    	
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/GTE/AVSZ/GTEAVSZ.exe"); // TODO; Failed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/GTE/NCLIP/GTENCLIP.exe"); // TODO; Failed
			
	    	// GPU - 16 BPP
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/MemoryTransfer/MemoryTransfer16BPP.exe"); // TODO; Only shows arrows
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderLine/RenderLine16BPP.exe"); // TODO; Don't have line rendering support
	    	std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderPolygon/RenderPolygon16BPP.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderPolygonDither/RenderPolygonDither16BPP.exe"); // TODO; Wrong colors(implement dither)
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderRectangle/RenderRectangle16BPP.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderTexturePolygon/15BPP/RenderTexturePolygon15BPP.exe"); // TODO;
	    	
	    	// Other stuff
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/Demo/printgpu/PRINTGPU.exe");
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/HELLOWORLD/16BPP/HelloWorld16BPP.exe"); // Passed
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/HELLOWORLD/24BPP/HelloWorld24BPP.exe"); // TODO; Squished
			
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/ImageLoad/ImageLoad.exe"); // TODO; Just black?
	    	
	    	// Requires controller
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/psxtest_cpx.exe");
	    	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CUBE/CUBE.exe");
	    	
	    	Exe exe;
	    	memcpy(&exe, data.data(), sizeof(exe));
			
	    	if(exe.tSize > data.size() - 0x800) {
	    		std::cerr << "Invalid exe size";
	    		exe.tSize = data.size() - 0x800;
	    	}
			
	    	for(uint32_t j = 0; j < exe.tSize; j++) {
	    		cpu.interconnect.store<uint8_t>(exe.tAddr + j, data[0x800 + j]);
	    	}
			
	    	cpu.pc = exe.pc0;
	    	cpu.nextpc = exe.pc0 + 4;
	    	//cpu->currentpc = cpu->pc;
			
	    	cpu.set_reg(28, exe.gp0);
			
	    	if(exe.sAddr != 0) {
	    		cpu.set_reg(29, exe.sAddr + exe.sSize);
	    		cpu.set_reg(30, exe.sAddr + exe.sSize);
	    	}
			
	    	cpu.branchSlot = false;
	    }
		
	    x++;
        
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT)
                break;
			
        	/*switch(event.type) {
        	case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				cpu->interconnect->_joypad.handleButtonEvent(event.cbutton);
				
        		break;
        	case SDL_CONTROLLERAXISMOTION:
        		cpu->interconnect->_joypad.handleAxisEvent(event.caxis);
				
        		break;
        	case SDL_JOYDEVICEADDED:
        	case SDL_JOYDEVICEREMOVED:
				cpu->interconnect->_joypad.updateConnectedControllers();
        		
        		break;
        	}*/
        }
    }
	
	SDL_Quit();
    
    return 0;
}
