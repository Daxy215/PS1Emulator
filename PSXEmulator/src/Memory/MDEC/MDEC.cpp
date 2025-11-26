#include "MDEC.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

MDEC::MDEC() {
    control.reg = 0;
    status.reg = 0x80040000;
    command.reg = 0;
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            scaleZag[zigzag[x+y*8]] = scaleFactor[x] * scaleFactor[y] / 8;
        }
    }
    
    for (int i = 0; i < 64; i++) {
        zagzig[zigzag[i]] = i;
    }
}

uint32_t MDEC::load(uint32_t addr) {
    if (addr == 0x1F801820) {
        // 1F801824h - MDEC1 - MDEC Status Register (R)
        
        // (or Garbage if there's no data available)
        // Idk what 'garbage' would be but ig a random value
        // I'm assuming it just returns a random memory address
        // so this is 100% not needed, but I'm bored
        if (output.empty())
            return rand() % INT32_MAX;
        
        // UHH TODO; ;-; Only part left but im so confused
        
        uint32_t data = 0;
        
        // Uhh TODO; Handle different types of bits somehow
        data = (output[outputIndex] & 0xFFFF) | (output[outputIndex + 1]  & 0xFFFF);
        outputIndex++;
        
        if (outputIndex > output.size()) {
            output.clear();
        }
        
        return data;
    } else if (addr == 0x1f801824) {
        // 1F801824h - MDEC1 - MDEC Status Register (R)
        
        // Data-Out Fifo Empty (0=No, 1=Empty)
        status.DataOutFifoEmpty = output.empty();
        
        // Data-In Fifo Full   (0=No, 1=Full, or Last word received)
        status.DataInFifoFull   = !input.empty();
        
        // (0=Ready, 1=Busy receiving or processing parameters)
        status.CommandBusy      = !output.empty();
        printf("RETURND; %x\n", status.reg);
        
        return status.reg;
    }
    
    return 0;
}

void MDEC::store(uint32_t addr, uint32_t val) {
    if (addr == 0x1f801820) {
        // 1F801820h - MDEC0 - MDEC Command/Parameter Register (W)
        if (paramCount != 0) {
            if (paramCount == 1) {
                printf("sadg\n");
            }
            
            handleCommandProcessing(val);
            
            counter++;
            paramCount--;
            status.ParameterWordsRemaining = (paramCount - 1) & 0xFFFF;
        } else {
            command.reg = val;
            handleCommand();
            counter = 0;
        }
    } else if (addr == 0x1f801824) {
        /**
         * 1F801824h - MDEC1 - MDEC Control/Reset Register (W)
         * 31    Reset MDEC (0=No change, 1=Abort any command, and set status=80040000h)
         * 30    Enable Data-In Request  (0=Disable, 1=Enable DMA0 and Status.bit28)
         * 29    Enable Data-Out Request (0=Disable, 1=Enable DMA1 and Status.bit27)
         * 28-0  Unknown/Not used - usually zero
         */
        
        control.reg = val;
        
        // How tf do I enable DMA0 and DMA1 wtf
        if (control.EnableDataInRequest) {
            // Enable Data-In Request  (0=Disable, 1=Enable DMA0 and Status.bit28)
            status.DataInRequest = true;
        }
        
        if (control.EnableDataOutRequest) {
            // Enable Data-Out Request (0=Disable, 1=Enable DMA1 and Status.bit27)
            status.DataOutRequest = true;
        }
        
        if (control.ResetMDEC) {
            // Reset MDEC (0=No change, 1=Abort any command, and set status=80040000h)
            status.reg = 0x80040000;
            
            // Abort command
            outputIndex = 0;
            command.reg = 0;
            paramCount = 0;
        }
    } else {
        printf("Unhandled store MDEC %x = %x\n", addr, val);
    }
}

void MDEC::handleCommand() {
    /**
    * Used to send command word, followed by parameter words to the MDEC
    * (usually, only the command word is written to this register,
    * and the parameter words are transferred via DMA0).
    */
    
    /**
     * 31-29 Command (1=decode_macroblock)
     * 28-27 Data Output Depth  (0=4bit, 1=8bit, 2=24bit, 3=15bit)      ;STAT.26-25
     * 26    Data Output Signed (0=Unsigned, 1=Signed)                  ;STAT.24
     * 25    Data Output Bit15  (0=Clear, 1=Set) (for 15bit depth only) ;STAT.23
     * 24-16 Not used (should be zero)
     * 15-0  Number of Parameter Words (size of compressed data)
     */
    
    const uint32_t cmd = command.Op;
    
    status.CommandBusy = true;
    
    // TODO; Unsure for those values????
    // It says 'reflects' so I'm assuming,
    // ( Command bits 25-28 are reflected to Status bits 23-26 as usually. ) 'as usually'???
    // ( Command bits 0-15 are reflected to Status bits 0-15 )
    // its meant to be copied to 'status'?????
    //status.DataOutputBit15  = command.DataOutputBit15;
    //status.DataOutputSigned = command.DataOutputSigned;
    //status.DataOutputDepth  = command.DataOutputDepth;
    
    switch (cmd) {
        case 0:
        // Those behave same as cmd (0)
        case 4:
        case 5:
        case 6:
        case 7:{
            // No function
            
            paramCount = 0;//command.NumberOfParameterWords;
            
            break;
        }
        
        case 1: {
            // MDEC(1) - Decode Macroblock(s)
            
            paramCount = command.NumberOfParameterWords;
            
            input.resize(paramCount * 2);
            output.resize(0);
            
            outputIndex = 0;
            
            break;
        }
        
        case 2: {
            // MDEC(2) - Set Quant Table(s)
            /**
             * 31-29 Command (2=set_iqtab)
             * 28-1  Not used (should be zero)  ;Bit25-28 are copied to STAT.23-26 though
             * 0     Color   (0=Luminance only, 1=Luminance and Color)
             */
            
            // TODO; Uhh if 28-1 should be zero...
            // ig that means rest bits 23-26 of status???
            //status.DataOutputBit15  = 0;
            //status.DataOutputSigned = 0;
            //status.DataOutputDepth  = 0;
            
            color = command.reg & 0x01;
            
            /**
             * The command word is followed by 64 unsigned parameter bytes,
             * for the Luminance Quant Table (used for Y1..Y4),
             * and if Command.Bit0 was set,
             * by another 64 unsigned parameter bytes for the Color Quant Table (used for Cr and Cb).
             */
            if (!color) {
                paramCount = 64 / 4; // 64 uint8_t
            } else {
                // 2 * 64
                paramCount = 128 / 4; // 128 uint8_t
            }
            
            break;
        }
        
        case 3: {
            // MDEC(3) - Set Scale Table
            
            /**
             * The command is followed by 64 signed halfwords with 14bit fractional part,
             * the values should be usually/always the same values
             * (based on the standard JPEG constants, although, MDEC(3) allows to use other values than that constants).
             */
            
            paramCount         = 64 / 4;
            
            break;
        }
        default: {
            printf("Unhandled MDEC Command %d\n", cmd);
        }
    }
    
    // Doesn't mention cmd (3) should 'reflect'
    //if (cmd == 3)
    //    return;
    
    // It only says it should copy,
    // remaining words if cmd = 0 | 1?
    // TODO; confirm this somehow
    
    // (4..7) mirrors of (0)
    //if (cmd != 3 && cmd != 2)
        // Only command (1) has minus 1 effect
        status.ParameterWordsRemaining = /*command.NumberOfParameterWords*/(paramCount - (cmd == 1 ? 1 : 0)) & 0xFFFF;
}

void MDEC::handleCommandProcessing(uint32_t val) {
    switch (command.Op) {
        case 1: {
            // MDEC(1) - Decode Macroblock(s)
            input[counter * 2] = val & 0xffff;
            input[counter * 2 + 1] = (val >> 16) & 0xffff;
            
            if (paramCount == 1) {
                decodeBlocks();
                input.clear();
            }
            
            break;
        }
        
        case 2: {
            // MDEC(2) - Set Quant Table(s)
            
            uint8_t base = counter * 4;
            uint8_t* table = luminanceQuantTable.data();
            
            /*if (counter < 64 / 4) {
                // Use Luminance Quant Table
                
            } else */if (counter < 128 / 4) {
                // Use Color Quant Table
                
                // It does say if this was set,
                // THEN.. it'll send another 64?
                // or does it not matter? idk ;-;
                // OK ig confirmed in their pseudocode;
                /*
                 * iqtab_core(iq_y,src), src=src+64       ;luminance quant table
                 * if command_word.bit0=1
                 *     iqtab_core(iq_uv,src), src=src+64    ;color quant table (optional)
                 * endif
                 */
                assert(color);
                
                table = colorQuantTable.data();
                base = (counter - (64 / 4)) * 4;
            } else {
                // Shouldn't happen ig?
                assert(false);
                break;
            }
            
            // Uhh each value in table is 4 bytes?
            for (int i = 0; i < 4; i++) {
                table[base + i] = val >> (i * 8);
            }
            
            break;
        }
        
        case 3: {
            // MDEC(3) - Set Scale Table
            
            // This command defines the IDCT scale matrix, which should be usually/always:
            /**
             *  5A82 5A82 5A82 5A82 5A82 5A82 5A82 5A82
             * 7D8A 6A6D 471C 18F8 E707 B8E3 9592 8275
             * 7641 30FB CF04 89BE 89BE CF04 30FB 7641
             * 6A6D E707 8275 B8E3 471C 7D8A 18F8 9592
             * 5A82 A57D A57D 5A82 5A82 A57D A57D 5A82
             * 471C 8275 18F8 6A6D 9592 E707 7D8A B8E3
             * 30FB 89BE 7641 CF04 CF04 7641 89BE 30FB
             * 18F8 B8E3 6A6D 8275 7D8A 9592 471C E707
             */
            for (int i = 0; i < 2; i++) {
                scaleTable[counter * 2 + i] = val >> (i * 16);
            }
            
            break;
        }
        
        default: {
            break;
        }
    }
}

void MDEC::reset() {
    // TODO;
    // Reset MDEC (0=No change, 1=Abort any command, and set status=80040000h)
    status.reg = 0x80040000;
    control = Control(0);
    command = DecodeCommand(0);
    
    // Abort command
    outputIndex = 0;
    command.reg = 0;
    paramCount = 0;
    
    output.clear();
    input.clear();
    blocks.fill(DCTBlock());
    
}

void MDEC::decodeBlocks() {
    for (auto src : input) {
        // Uhh
        DCTBlock block = decodeMarcoBlocks(src);
        
        //output.insert(output.end(), (block.data).begin(), (block.data).end());
        for (auto RLE : block.data) {
            output.push_back(RLE.reg);
        }
    }
}

MDEC::DCTBlock MDEC::decodeMarcoBlocks(uint16_t &src) {
    DCTBlock block;
    
    if (command.DataOutputDepth > 1) {
        // 15bpp or 24bpp depth
        // decode_colored_macroblock
        rl_decode_block(Crblk.data(), src, iq_uv.data()); // ;Cr (low resolution)
        rl_decode_block(Cbblk.data(), src, iq_uv.data()); // ;Cb (low resolution)
        
        // ;Y1 (and upper-left  Cr,Cb)
        rl_decode_block(Yblk .data(), src, iq_y .data());
        yuv_to_rgb(block, 0, 0);
        
        // ;Y2 (and upper-right Cr,Cb)
        rl_decode_block(Yblk .data(), src, iq_y .data());
        yuv_to_rgb(block, 0, 8);
        
        // ;Y3 (and lower-left  Cr,Cb)
        rl_decode_block(Yblk .data(), src, iq_y .data());
        yuv_to_rgb(block, 8, 0);
        
        // ;Y4 (and lower-right Cr,Cb)
        rl_decode_block(Yblk .data(), src, iq_y .data());
        yuv_to_rgb(block, 8, 8);
    } else {
        // 4bpp or 8bpp depth
        // decode_monochrome_macroblock
        rl_decode_block(Yblk.data(), src, iq_y.data());
        y_to_mono(block); // ;Y
        
        assert(false);
    }
    
    return block;
}

void MDEC::rl_decode_block(uint16_t *blk, uint16_t &src, uint16_t *qt) {
    uint16_t n, k = 0;
    uint16_t q_scale = 0;
    uint16_t val = 0;
    
    // First value is DCT
    
    skip:
        // To avoid confusion
        n = src++;
        
        // Skip padding
        if (n == 0xFE00)
            goto skip;
        
        q_scale = (n >> 10) & 0x3F; // Contains scale value (not 'skip' value)
        
        // calc first value (without q_scale/8) (?)
        // https://fgiesen.wordpress.com/2024/10/23/zero-or-sign-extend/
        // https://www.geeksforgeeks.org/c/sign-extend-a-nine-bit-number-in-c/
        val = n & 0x3FF; // 10 bits
        if (val & 0x200) val |= ~0x3FF; // If signbit sent, extend
        
        val = val * qt[k];
    
    // TODO; Uhh this is wrong
    for (int i = 0; i < 64; i++) {
        lop:
            if (q_scale == 0) {
                // Breh
                val = n & 0x3FF; // 10 bits
                if (val & 0x200) val |= ~0x3FF; // If signbit sent, extend
                
                val = val * 2;
            }
            
            val = std::min(0x3FF, std::max(-0x400, static_cast<int>(val)));
            
            // Used for 'fast_idct_core' only
            //val = val * scaleZag[i];
            
            // Store entry normally
            if (q_scale > 0) {
                blk[zagzig[k]] = val;
            }
            
            // Store entry special, no zigzag
            else if (q_scale == 0) {
                blk[k] = val;    
            }
            
            n = src;
            src += 2;
            
            // get next entry (or FE00h end code)
            if (n == 0xFE00)
                break;
            
            // Breh x2
            k = k + ((n >> 10) & 0x3F) + 1; // Skip zerofilled entries
            val = n & 0x3FF; // 10 bits
            if (val & 0x200) val |= ~0x3FF; // If signbit sent, extend
            
            val = (val * qt[k] * q_scale) / 8;
        
        // ;should end with n=FE00h (that sets k>63)
        if (k <= 63) {
            goto lop;
        }
    }
    
    real_idct_core(blk);
    
    // return (with "src" address advanced)
    src++;
}

void MDEC::fast_idct_core(uint16_t *blk) {
    
}

void MDEC::real_idct_core(uint16_t *blk) {
    std::array<uint16_t, 64> tmp {};
    
    uint16_t* src = blk;
    uint16_t* dst = tmp.data();
    
    for (int pass = 0; pass < 2; pass++) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                int32_t sum = 0;
                
                for (int z = 0; z < 8; z++) {
                    sum += src[y + z*8] * (scaleTable[x + z*8] / 8);
                }
                
                dst[x+y*8] = (sum + 0x0FFF) / 0x2000;
            }
        }
        
        // Swap src(blk) -> dst (temp buffer)
        std::swap(src, dst);
    }
    
    if (src != blk) {
        std::memcpy(blk, src, 64 * sizeof(uint16_t));
    }
}

void MDEC::yuv_to_rgb(DCTBlock& block, uint16_t xx, uint16_t yy) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            double R = Crblk[(x + xx) / 2 + ((y + yy) / 2) * 8];
            double B = Cbblk[(x + xx) / 2 + ((y + yy) / 2) * 8];
            double G = (B * -0.3437) + (R * -0.7143);
            
            R = (R * 1.402);
            B = (B * 1.772);
            
            const uint32_t Y = Yblk[x + y * 8];
            
            // According to this https://godbolt.org/z/cbfK9E998
            // from the 'sporule' guy:
            // https://www.reddit.com/r/cpp/comments/1980q8l/stdclamp_still_generates_less_efficient_assembly/
            // this uses 1 less instruction than std::clamp
            // no idea what ANY of whatever the person said, but I agree with him
            // std::min(max, std::max(min, v))
            R = std::min(127, std::max(-128, static_cast<int>(Y + R)));
            G = std::min(127, std::max(-128, static_cast<int>(Y + G)));
            B = std::min(127, std::max(-128, static_cast<int>(Y + B)));
            
            // Convert back to halfwords
            auto r = static_cast<uint16_t>(R);
            auto g = static_cast<uint16_t>(G);
            auto b = static_cast<uint16_t>(B);
            
            // Uhh unsure whether to use 'status' or 'command' here..
            // but I suppose 'command' makes sense? But what do I know lol
            // Data Output Signed (0=Unsigned, 1=Signed)
            if (command.DataOutputSigned == 0) {
                r ^= 0x80;
                g ^= 0x80;
                b ^= 0x80;
            }
            
            uint16_t R5 = (r & 0xFF) >> 3;
            uint16_t G5 = (g & 0xFF) >> 3;
            uint16_t B5 = (b & 0xFF) >> 3;
            
            uint16_t pixel = (R5 << 10) | (G5 << 5) | B5;
            block.data[(x + xx) + (y + yy) * 16] = RLE(pixel);
        }
    }
}

void MDEC::y_to_mono(DCTBlock &block) {
    for (int i = 0; i < 64; i++) {
        uint16_t Y = Yblk[i];
        
        Y = Y & 0x1FF; // Clip to signed 9bit range
        
        // Saturate from 9bit to signed 8bit range
        Y = std::min(127, std::max(-128, static_cast<int>(Y)));
        
        // Data Output Signed (0=Unsigned, 1=Signed)
        if (command.DataOutputSigned == 0) {
            Y ^= 0x80;
        }
        
        block.data[i] = RLE(Y);
    }
}
