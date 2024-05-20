// Core.cpp

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "Emulator/Bios.h"
#include "Emulator/CPU.h"
#include "Emulator/interconnect.h"

// Page 27

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ! Please note most of the stuff here isn't really written by me,    !
// ! I mean the comments of course.. I am only using it as a reminder, !
// ! or a quick lookup table or of the sorts.                          !
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// References
// https://github.com/deadcore/playstation-emulator/blob/master/src/cpu/mod.rs
// https://www.reddit.com/r/EmuDev/comments/hvb042/how_to_write_an_ps1_emulator/
// The most savour of ALL https://problemkaputt.de/psx-spx.htm#cpuspecifications
// https://en.wikipedia.org/wiki/NOP // For null functions etc..
// https://en.wikipedia.org/wiki/R3000
//

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
 * The PS1 uses little-endian formation for storing data.
    * As they store the least significant byte first.
    
 * MIPS R3000 CPU can support up to 4 coprocessors:
    * Coprocessor 0 (cop0):
        * Is basically used for exception handling.
        
    * Coprocessor 1 (cop1):
        * Is an optional coprocessor that is available for floating points arithmetic usage.
        
    * Coprocessor 2 (cop2):
        * Seems to be custom made for the Playstation? However, it's called,
        * Geometry Transformation Engine (GTE). Used for... 3D stuff, such as,
        * perspectivies, transformations, matrixes etc..
        
    * Coprocessor 3 (cop3):
        * This one isn't really implemented on the Playstation.
        
    * Note to future-self!
        * Guide will probably never use 3D stuff, so... GLHF 
 */

// Tables; THEY AREN'T MADE BY ME!  ¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?¬?
// I mostly obtained them through the guide.

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

int main(int argc, char* argv[]) {
    Bios* bios = new Bios("BIOS/ps-22a.bin");
    
    Interconnect* inter = new Interconnect(bios);
    
    CPU* cpu = new CPU(inter);
    
    while(true) {
        cpu->executeNextInstruction();
    }
    
    return 0;
}