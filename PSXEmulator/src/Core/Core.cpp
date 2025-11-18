#include <iostream>
#include <chrono>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../CPU/CPU.h"
#include "../Memory/IO/SIO.h"

//#include "../Memory/Bios/Bios.h"

#include "../Utils/FileSystem/FileManager.h"
#include "../GPU/Rendering/Renderer.h"

#include <filesystem>
#include <algorithm>

#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif
// To avoid, "gl.h included before "glew.h"
#include <GL/glew.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ! Please note most of the stuff here isn't really written by me,    !
// ! I mean the comments, of course.. I am only using it as a reminder !
// ! or a quick lookup table or of sorts.                              !
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// References
// https://github.com/deadcore/playstation-emulator/blob/master/src/cpu->mod.rs
// https://www.reddit.com/r/EmuDev/comments/hvb042/how_to_write_an_ps1_emulator/
// The most savour of ALL https://problemkaputt.de/psx-spx.htm#cpuspecifications
// https://en.wikipedia.org/wiki/NOP // For null functions etc..
// https://en.wikipedia.org/wiki/R3000
// https://www.dsi.unive.it/~gasparetto/materials/MIPS_Instruction_Set.pdf
// https://github.com/Lameguy64/PSn00bSDK
//
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
    * Field bits        Description-
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
            * move $a0, $v0,
        * However, the "move" instruction is not part of the MIPS instruction set.
        * It's just a convenient shorthand understood by the assembler which,
        * which will generate the equivalent instruction;
            * addu $a0, $v0, $zero
        * We can see that it is effectively doing the same thing as the previous instruction,
        * but we are avoiding implementing a dedicated "move" instruction in the CPU.
        
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
    * 
    * Dont get ANY of this. Page 16
 */

// I was too lazy so I stole this,
// from somewhere, and I really,
// can't remember from where..
// Idk why I didn't link it
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

std::unique_ptr<Emulator::Gpu> gpu;
std::unique_ptr<CPU> cpu;

/*std::vector<std::string> testPaths;
int currentIndex = 0;
bool loadNextTest = true;*/

void handleLoadExe(std::string path) {
	std::cerr << "Loading test EXE file\n";
	
	using namespace Emulator::Utils;
	
	//std::vector<uint8_t> data = FileManager::loadFile(path);
	
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
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CPUTest/CPU/LOADSTORE/LB/CPULB.exe"); // Passed
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
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/MemoryTransfer/MemoryTransfer16BPP.exe"); // Passed ig?
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderLine/RenderLine16BPP.exe"); // TODO; Don't have line rendering support
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderPolygon/RenderPolygon16BPP.exe"); // Passed
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderPolygonDither/RenderPolygonDither16BPP.exe"); // TODO; Wrong colors(implement dither)
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderRectangle/RenderRectangle16BPP.exe"); // Passed
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/GPU/16BPP/RenderTexturePolygon/15BPP/RenderTexturePolygon15BPP.exe"); // Passed
	
	// Other stuff
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/Demo/printgpu/PRINTGPU.exe");
	///std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/HELLOWORLD/16BPP/HelloWorld16BPP.exe"); // Passed
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/HELLOWORLD/24BPP/HelloWorld24BPP.exe"); // TODO; Unsupported format
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/PSX-master/Demo/vblank/VBLANK.exe");
	
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/ImageLoad/ImageLoad.exe"); // Passed
	
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/psxtest_cpu.exe");
	
	// Requires controller
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/psxtest_cpx.exe");
	std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/psxtest_gpu.exe");
	
	// It's drawing the cube(obviously no textures),
	// though, idk where im messing up bc its never checking,
	// for the controller's inputs. So, I can't really fully test it..
	// Future me; I was causing the wrong interrupt
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/PSX-master/CUBE/CUBE.exe");
	
	// CDROM Tests
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/cdrom/getloc/getloc.exe"); // TODO; Failed
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/cdrom/cdltest.ps-exe");
	
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/ps1-tests/cpu->access-time/access-time.exe"); // TODO; All timings return 4
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/cpu->code-in-io/code-in-io.exe"); // TODO; Too many unimplemented things
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/cpu->cop/cop.exe"); // TODO; Fails some tests?
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/cpu->io-access-bitwidth/io-access-bitwidth.exe"); // TODO; Fails many tests
	
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/dma/otc-test/otc-test.exe"); // TODO; Fails many tests
	
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/animated-triangle/animated-triangle.exe"); // TODO; Needs GTE
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/bandwidth/bandwidth.exe"); // TODO; speed: 20000-30000 MB/s lol
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/benchmark/benchmark.exe"); // Just a benchmark.. Not really a test
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/quad/quad.exe"); // Ig pases?
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/version-detect/version-detect.exe"); // Not really a tested but I suppose (0* GPU version 2 [New 208pin GPU (LATE-PU-8 and up)])
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/rectangles/rectangles.exe"); // TODO; Unhandled commands
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/ps1-tests/gpu/triangle/triangle.exe");
	
	//std::vector<uint8_t> data = FileManager::loadFile("ROMS/Tests/ps1-tests/input/pad/pad.exe");
	
	/**
	 * Idk where exactly the cause but,
	 * it seems like it's calling "oplwc2",
	 * and the address isn't aligned to,
	 * 4 bytes, so at least from my knowleagde,
	 * I am causing a LoadAddressError expection,
	 * howerver, this is causing the emulator,
	 * to go in some sort of a hault? No idea,
	 * after commenting out the expection cause,
	 * it seems to run? But all writes to "oplwc2",
	 * aren't aligned? Most likely an issue with my CPU,
	 * could be that I have many unimlemented memory locations?
	 * 
	 * The test seems to fully run while it's commented out.
	 * 
	 * Okay so I found out that the issue IS actually caused by,
	 * the timers being wrong or the VBlank interrupt.
	 */
	//std::vector<uint8_t> data = Emulator::Utils::FileManager::loadFile("ROMS/Tests/ps1-tests/timers/timers.exe");
	
	Exe exe;
	memcpy(&exe, data.data(), sizeof(exe));
	
	if (exe.tSize > data.size() - 0x800) {
		std::cerr << "Invalid exe size";
		exe.tSize = data.size() - 0x800;
	}
	
	for (uint32_t j = 0; j < exe.tSize; j++) {
		cpu->interconnect.store<uint8_t>(exe.tAddr + j, data[0x800 + j]);
	}
	
	cpu->pc = exe.pc0;
	cpu->nextpc = exe.pc0 + 4;
	//cpu->currentpc = cpu->pc;
	
	cpu->set_reg(28, exe.gp0);
	
	if (exe.sAddr != 0) {
		cpu->set_reg(29, exe.sAddr + exe.sSize);
		cpu->set_reg(30, exe.sAddr + exe.sSize);
	}
	
	cpu->branchSlot = false;
}

void rest(const std::string& biosPath) {
	cpu->reset();
}

void runFrame() {
	while(true) {
		for(int i = 0; i < 100; i++) {
			//if (cpu->pc != 0x80030000) {
			if (true) {
				cpu->executeNextInstruction();
				
				/*if (!cpu->paused) {
					cpu->executeNextInstruction();
				} else if (cpu->stepRequested) {
					cpu->executeNextInstruction();
					cpu->stepRequested = false;
				} else if (cpu->stepUntilBranchTakenRequested) {
					auto pc = cpu->pc;
					
					cpu->executeNextInstruction();
					
					if (pc + 4 != cpu->pc) {
						cpu->stepUntilBranchTakenRequested = false;
					}
				} else if (cpu->stepUntilBranchNotTakenRequested) {
					auto pc = cpu->pc;
					
					cpu->executeNextInstruction();
					
					if (pc + 4 == cpu->pc) {
						cpu->stepUntilBranchNotTakenRequested = false;
					}
				}*/
			} else {
				if(false) {
					// Skip bios logo
					printf("Skipping bootrom\n");
					cpu->pc = cpu->reg(31);
					cpu->nextpc = cpu->pc + 4;
				} else {
					handleLoadExe(""/*testPaths[currentIndex]*/);
				}
			}
		}
		
		if(cpu->interconnect.step(300)) {
			IRQ::trigger(IRQ::VBlank);
			
			break;
		}
	}
}

namespace fs = std::filesystem;

struct FileInfo {
    std::string name;
    std::string path;
    bool is_directory;
};

static void ShowFileBrowser(bool* p_open, CPU* cpu) {
    static std::string current_dir = fs::current_path().string();
    static std::vector<FileInfo> files;
    static std::string selected_file;
    static std::vector<std::string> history;
	
	Refresh:
		files.clear();
    		
		try {
			for (const auto& entry : fs::directory_iterator(current_dir)) {
				files.push_back({
					entry.path().filename().string(),
					entry.path().string(),
					entry.is_directory()
				});
			}

			std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
				if (a.is_directory != b.is_directory) 
					return a.is_directory > b.is_directory;
				return a.name < b.name;
			});
		} catch (...) {
				
		}
    
    // Refresh directory contents
    if (ImGui::Button("Refresh")) {
        goto Refresh;
    }
    
    // Navigation buttons
    ImGui::SameLine();
    if (ImGui::Button("Up") && current_dir != fs::path(current_dir).root_path()) {
        history.push_back(current_dir);
        current_dir = fs::path(current_dir).parent_path().string();
		
        goto Refresh;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Home")) {
        history.push_back(current_dir);
        current_dir = fs::path(fs::current_path().root_path()).string();
		
        goto Refresh;
    }

    // Current directory path
    ImGui::Text("Current Directory: %s", current_dir.c_str());
    
    // File list
    if (ImGui::BeginListBox("##FileList", ImVec2(-FLT_MIN, 20 * ImGui::GetTextLineHeightWithSpacing()))) {
        for (const auto& file : files) {
            const bool is_selected = (selected_file == file.path);
            ImGui::PushID(file.path.c_str());
            
            if (ImGui::Selectable(file.name.c_str(), is_selected)) {
                selected_file = file.path;
            	
                if (file.is_directory) {
                    history.push_back(current_dir);
                    current_dir = file.path;
					
					goto Refresh;
                }
            }
            
            if (file.is_directory) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "[DIR]");
            }
            
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    // File filter
    static char filter[32] = ".exe,.bin";
    ImGui::InputText("Filter", filter, IM_ARRAYSIZE(filter));

    // Load buttons
    if (ImGui::Button("Load Selected") && !selected_file.empty()) {
        fs::path path(selected_file);
        cpu->reset();
        
        if (path.extension() == ".exe") {
            handleLoadExe(selected_file);
        } else if (path.extension() == ".bin") {
            cpu->interconnect._cdrom.swapDisk(selected_file);
        }
        
        *p_open = false;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
    }
}

static bool show_file_browser = false;

int main(int argc, char* argv[]) {
	// Was used for debuging
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	// Initialise everything
	
	/**
	 * TODO;
	 * So, not sure if this is correct but,
	 * sometimes, the GPU is drawing outside,
	 * the "main" drawing area, and it seems that,
	 * the CPU tries to fetch from those, but to me,
	 * I am drawing them using OpenGL so I'm not really sure,
	 * what I should be doing to solve this..
	 */
	/**
	 * TODO; Dithering isn't implemented
	 * TODO; Transparency isn't implemented
	 * TODO; Line rendering isn't implemented
	 * TODO; Offset bug? Not exactly sure what it is
	 * TODO; Blending-textures?
     * TODO; VRAM issue(DMA), sometimes DMA tries to fetch from a polygon but it doesn't exists.. As it isn't in the VRAM
     * TODO; Copying parameters from textures not implemented
	 * TODO; Sometimes, GPU draws an image in the display area of the VRAM. Ensure to render the pixels using VBOS as well
	 */
    gpu = std::make_unique<Emulator::Gpu>();
    
    // TODO; Implement(probably won't implement for a very while)
    //Emulator::SPU spu = Emulator::SPU();
	
    cpu = std::make_unique<CPU>(Interconnect(gpu.get()));
	
	/*cpu->interconnect._cdrom = psx_cdrom_create();
	cpu->interconnect._cdrom->disc = psx_disc_create();
	psx_cdrom_init(cpu->interconnect._cdrom,
			[](void*) {
							IRQ::trigger(IRQ::Interrupt::CDROM);
						}, nullptr);
	
	psx_cdrom_set_version(cpu->interconnect._cdrom, CDR_VERSION_C0A);
	psx_cdrom_set_region(cpu->interconnect._cdrom, CDR_REGION_EUROPE);
	
	psx_disc_open(cpu->interconnect._cdrom->disc, "ROMS/Run Crash/Desire_-_Run_Crash_(PSX).cue");*/
	
	// TODO; For now, manually load in disc
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Run Crash/Desire_-_Run_Crash_(PSX).cue");
	
	cpu->interconnect._cdrom.swapDisk("../ROMS/Crash Bandicoot (Europe, Australia)/Crash Bandicoot (Europe, Australia).cue");
	//cpu->interconnect._cdrom.swapDisk("ROMS/Battle Arena Toshinden (Europe)/Battle Arena Toshinden (Europe).cue");
	//cpu->interconnect._cdrom.swapDisk("ROMS/Sonic Wings Special (Europe)/Sonic Wings Special (Europe)/Sonic Wings Special (Europe).cue");
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Tetris X/Tetris X (Japan).cue");
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Ridge Racer (Europe)/Ridge Racer (Europe).cue");
	
	// Uhh works somehow idek how BUT obviously GPU bug but I think it's actually GTE
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Tekken 3 (USA)/Tekken 3 (USA).cue");
	
	/**
	 * Had an issue with the controller but now it's fixed,
	 * TODO; Missing some GP0 commands
	 * GP0(48h) - Monochrome Poly-line, opaque
	 */
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Pink Panther - Pinkadelic Pursuit (Europe) (En,Fr,De,Es,It)/Pink Panther - Pinkadelic Pursuit (Europe) (En,Fr,De,Es,It).cue");
	
	/**
	 * Also had controller issues.
	 */
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Crash Bandicoot - Warped (USA)/Crash Bandicoot - Warped (USA).cue");
	
	// Works but need to skip all cut scenes to see anything(dont have MDEC)
	// TODO; Uses line rendering but doesn't crash
	// TODO; Uses CDROM (0x14 and 0x13)
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Pepsiman (Japan)/Pepsiman (Japan).cue");
	
	// Games that are broken
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Yu-Gi-Oh! Forbidden Memories (Europe)/Yu-Gi-Oh! Forbidden Memories (Europe).cue"); // TODO; Missing CDROM(0x20/0x1E/0x16) SUB(0x04)
	//cpu->interconnect._cdrom.swapDisk("../ROMS/This Is Football (Europe)/This Is Football (Europe).cue"); // TODO; CDROM(0x11)
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Crash Bash (Europe) (En,Fr,De,Es,It)/Crash Bash (Europe) (En,Fr,De,Es,It).cue"); // TODO; CDROM(0x11)
	
	// Works but with some GPU bugs
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Grudge Warriors (Europe) (En,Fr,De,Es,It)/Grudge Warriors (Europe) (En,Fr,De,Es,It).cue");
	
	// TODO; Works completely fine BUT with so many GPU bugs XD
	//cpu->interconnect._cdrom.swapDisk("../ROMS/Twisted Metal 4 (USA) (Rev 1)/Twisted Metal 4 (USA) (Rev 1).cue");
	
	/*namespace fs = std::filesystem;
	
	std::string testRoot = "ROMS/Tests/PSX-master/CPUTest";
	
	for(const auto& entry : fs::recursive_directory_iterator(testRoot)) {
		if(entry.is_regular_file() && entry.path().extension() == ".exe") {
			testPaths.push_back(entry.path().string());
		}
	}
	
	std::sort(testPaths.begin(), testPaths.end());*/
	
	glfwSetKeyCallback(gpu->renderer->window, Emulator::IO::SIO::keyCallback);
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(gpu->renderer->window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	
	double fps = 0.0;
	int frames = 0;
	double frameTime = 0;
	std::chrono::steady_clock::time_point firstTime;
	std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
	double passedTime = 0;
	double unprocessedTime = 0;
	const double UPDATE_CAP = 1.0/600.0;
	
	bool render = false;
	
	/*while(true) {
		glfwPollEvents();
		runFrame();
	}*/
	
	while(!glfwWindowShouldClose(gpu->renderer->window)) {
		render = false;
		
		glfwPollEvents();
		
		firstTime = std::chrono::steady_clock::now();
		passedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(firstTime - lastTime).count() / 1000000000.0;//firstTime - lastTime;
		lastTime = firstTime;
		unprocessedTime += passedTime;
		frameTime += passedTime;
		
		while(unprocessedTime >= UPDATE_CAP) {
			unprocessedTime -= UPDATE_CAP;
			render = true;
			
			if(frameTime >= 1.0) {
				frameTime = 0;
				fps = frames;
				frames = 0;
				
				std::cerr << "FPS: " << std::to_string(fps) << " - " << std::to_string(gpu->frames) << "\n";
				
				gpu->frames = 0;
			}
 		}
		
		if (glfwGetWindowAttrib(gpu->renderer->window, GLFW_ICONIFIED)) {
			//ImGui_ImplGlfw_Sleep(10);
			continue;
		}
		
		// Start frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		if (render) {
			//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			runFrame();
			/*if(glfwGetKey(gpu->renderer->window, GLFW_KEY_N) == GLFW_PRESS && loadNextTest) {
				loadNextTest = false;
				
				if(currentIndex + 1 < testPaths.size()) {
					currentIndex++;
					
					rest("BIOS/ps-22a.bin");
					std::cerr << "Loaded test: " << testPaths[currentIndex] << "\n";
				} else {
					std::cerr << "All tests completed!\n";
				}
			}
			
			if(glfwGetKey(gpu->renderer->window, GLFW_KEY_N) == GLFW_RELEASE) {
				loadNextTest = true;
			}*/
			int width, height;
			glfwGetFramebufferSize(gpu->renderer->window, &width, &height);
			glViewport(0, 0, width, height);
		}
		
		//cpu->showDisassembler();
		
		/*if (ImGui::Begin("VRAM")) {
			ImGui::Image((ImTextureID)(intptr_t)gpu->vram->texImGui, ImVec2(1024, 512));
			ImGui::End();
		}*/
		
		int winW, winH;
		glfwGetFramebufferSize(gpu->renderer->window, &winW, &winH);
		
		float x1 = (gpu->drawingAreaLeft   / 1024.0f) * winW;
		float y1 = (gpu->drawingAreaTop    / 512.0f)  * winH;
		float x2 = (gpu->drawingAreaRight  / 1024.0f) * winW;
		float y2 = (gpu->drawingAreaBottom / 512.0f)  * winH;
		
		ImGui::GetForegroundDrawList()->AddRect(
			ImVec2(x1, y1),
			ImVec2(x2, y2),
			IM_COL32(255, 0, 0, 255),
			0.0f,
			0,
			3.0f
		);
		
		/*if (ImGui::Begin("Post-Processing Settings")) {
			ImGui::SeparatorText("Bloom Settings");
			ImGui::SliderFloat("Bloom Threshold", &gpu->renderer->threshold, -5.0f, 5.0f);
			ImGui::SliderFloat("Bloom Blur Radius", &gpu->renderer->blurRadius, 0.0f, 10.0f);
			ImGui::SliderInt("Bloom Passes", &gpu->renderer->bloomPasses, 0, 50);
			ImGui::SliderFloat("Bloom Intensity", &gpu->renderer->bloomIntensity, 0.0f, 5.0f);
			ImGui::Checkbox("Enable bloom", &gpu->renderer->enableBloom);
			
			ImGui::SeparatorText("Upscaling Quality");
			ImGui::Checkbox("Enable Upscaling", &gpu->renderer->enableUpscaling);
			ImGui::SliderInt("Sample Radius", &gpu->renderer->sampleRadius, 1, 16);
			ImGui::SliderFloat("LOD Bias", &gpu->renderer->lodBias, -1.0f, 1.0f);
			ImGui::SliderFloat("Kernel B", &gpu->renderer->kernelB, -1.0f, 1.0f);
			ImGui::SliderFloat("Kernel C", &gpu->renderer->kernelC, -1.0f, 1.0f);
			ImGui::SliderFloat("Sharpness", &gpu->renderer->sharpness, 0.0f, 5.0f);
			ImGui::SliderFloat("Edge Threshold", &gpu->renderer->edgeThreshold, 0.0f, 0.5f);
			ImGui::Checkbox("Adaptive Sharpening", &gpu->renderer->enableAdaptiveSharpening);
			
			ImGui::SeparatorText("Color Adjustments");
			ImGui::SliderFloat("Contrast", &gpu->renderer->contrast, 0.5f, 2.0f);
			ImGui::SliderFloat("Saturation", &gpu->renderer->saturation, 0.0f, 2.0f);
			ImGui::SliderFloat("Gamma", &gpu->renderer->gamma, 0.1f, 5.0f);
			
			ImGui::SeparatorText("CRT Effects");
			ImGui::SliderFloat("Scanline", &gpu->renderer->scanline, 0.0f, 1.0f);
			ImGui::SliderFloat("Halation", &gpu->renderer->halation, 0.0f, 0.3f);
			ImGui::SliderFloat("Dither Strength", &gpu->renderer->ditherStrength, 0.0f, 0.02f);
			ImGui::SliderFloat("Film Grain", &gpu->renderer->noiseStrength, 0.0f, 0.1f);
		}
		ImGui::End();*/
		
		/*if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open...")) {
					show_file_browser = true;
				}
				
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}
		
		if (show_file_browser) {
			ShowFileBrowser(&show_file_browser, cpu.get());
		}*/
		
		ImGui::Render();
		
		if (render) {
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(gpu->renderer->window);
			
			frames++;
		} else {
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	
	glfwDestroyWindow(gpu->renderer->window);
	glfwTerminate();
	
    return 0;
}
