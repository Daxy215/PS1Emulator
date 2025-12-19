#include "MDEC.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// https://github.com/Laxer3a/MDEC/blob/master/hdlMDEC/doc/PSX%20FPGA%20MDEC%20DOC.pdf

MDEC::MDEC() {
    reset();
    /*control.reg = 0;
    status.reg = 0x80040000;
    command.reg = 0;*/
    
    /*for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            scaleZag[zigzag[x + y * 8]] = scaleFactor[x] * scaleFactor[y] / 8;
        }
    }*/
    
    for (int i = 0; i < 64; i++) {
        zagzig[zigzag[i]] = i;
    }
}

uint32_t MDEC::load(uint32_t addr) {
    if (addr < 4) {
        // 1F801820h.Read - MDEC Data/Response Register (R)
        if (outputIndex > output.size()) {
            //printf("OUT OF BOUNDS! MODE; %d\n", status.DataOutputDepth);
            return 0;
        }
        
        // (or Garbage if there's no data available)
        if (output.empty() || outputIndex >= output.size())
            return 0;
        
        uint32_t v = 0;
        
        if (status.DataOutputDepth == 3) {
            if (outputIndex + 1 >= output.size())
                return 0;
            
            uint32_t lo = output[outputIndex++] & 0xFFFF;
            uint32_t hi = output[outputIndex++] & 0xFFFF;
            
            v = lo | (hi << 16);
        } else if (status.DataOutputDepth == 2) {
            v = output[outputIndex];
            outputIndex++;
        }
        
        if (outputIndex >= output.size()) {
            //printf("OUT OF2 BOUNDS! MODE; %d %d times. Data left; %lu\n", status.DataOutputDepth, amount++, (output.size() - outputIndex));
            output.clear();
            outputIndex = 0;
        }
        
        //return 0x7C007C00; // BLUE
        //return 0x001F001F; // RED
        return v;
    } else if (addr == 4) {
        // 1F801824h - MDEC1 - MDEC Status Register (R)
        
        status.DataOutFifoEmpty = output.empty();
        status.CommandBusy     = !status.DataOutFifoEmpty;
        
        // Data-Out Fifo Empty (0=No, 1=Empty)
        //status.DataOutFifoEmpty = output.empty();
        
        // Data-In Fifo Full   (0=No, 1=Full, or Last word received)
        status.DataInFifoFull   = !input.empty();
        
        // (0=Ready, 1=Busy receiving or processing parameters)
        //status.CommandBusy      = !output.empty();
        //printf("RETURND; %x\n", status.reg);
        
        return status.reg;
    }
    
    assert(false);
    
    return 0;
}

void MDEC::store(uint32_t addr, uint32_t val) {
    //printf("MDEC::store %x - %x\n", addr, val);
    
    if (addr < 4) {
        // 1F801820h - MDEC0 - MDEC Command/Parameter Register (W)
        if (paramCount != 0) {
            handleCommandProcessing(val);
            
            paramCount--;
            status.ParameterWordsRemaining = (paramCount/* - 1*/) & 0xFFFF;
        } else {
            command.reg = val;
            handleCommand();
            counter = 0;
        }
    } else if (addr == 4) {
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
            /*status.reg = 0x80040000;
            
            // Abort command
            outputIndex = 0;
            command.reg = 0;
            paramCount = 0;*/
            reset();
        }
    } else {
        printf("Unhandled store MDEC %x = %x\n", addr, val);
        assert(false);
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
    
    switch (cmd) {
        case 1: {
            // MDEC(1) - Decode Macroblock(s)
            
            paramCount = command.NumberOfParameterWords;
            
            input.clear();
            input.reserve(paramCount*2);
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
            paramCount = color ? 32 : 16;
            
            break;
        }
        
        case 3: {
            // MDEC(3) - Set Scale Table
            
            /**
             * The command is followed by 64 signed halfwords with 14bit fractional part,
             * the values should be usually/always the same values
             * (based on the standard JPEG constants, although, MDEC(3) allows to use other values than that constants).
             */
            
            paramCount = 32;
            
            break;
        }
        default: {
            assert(false && "Unhandled command");
        }
    }
    
    // Doesn't mention cmd (3) should 'reflect'
    //if (cmd == 3)
    //    return;
    
    // It only says it should copy,
    // remaining words if cmd = 0 | 1?
    if (cmd == 0 || cmd == 1) {
        // TODO; confirm this somehow
        // TODO; Unsure for those values????
        // It says 'reflects' so I'm assuming,
        // ( Command bits 25-28 are reflected to Status bits 23-26 as usually. ) 'as usually'???
        // ( Command bits 0-15 are reflected to Status bits 0-15 )
        // its meant to be copied to 'status'?????
        status.DataOutputBit15  = command.DataOutputBit15;
        status.DataOutputSigned = command.DataOutputSigned;
        status.DataOutputDepth  = command.DataOutputDepth;
    }
    
    // (4..7) mirrors of (0)
    //if (cmd != 3 && cmd != 2)
        // Only command (1) has minus 1 effect
        status.ParameterWordsRemaining = /*command.NumberOfParameterWords*/(paramCount - (cmd == 1 ? 1 : 0)) & 0xFFFF;
}

void MDEC::handleCommandProcessing(uint32_t val) {
    switch (command.Op) {
        case 1: {
            // MDEC(1) - Decode Macroblock(s)
            input.push_back(val         & 0xFFFF);
            input.push_back((val >> 16) & 0xFFFF);
            counter += 2;
            
            if (counter >= command.NumberOfParameterWords * 2) {
                decodeBlocks();
                input.clear();
                counter = 0;
            }
            
            break;
        }
        
        case 2: {
            // MDEC(2) - Set Quant Table(s)
            
            bool lumi = (counter >= 16);
            
            uint8_t* table = lumi
                ? colorQuantTable.data()
                : luminanceQuantTable.data();
            
            int zz = (counter % 16) * 4;
            
            table[zigzag[zz + 0]] = (val      ) & 0xFF;
            table[zigzag[zz + 1]] = (val >>  8) & 0xFF;
            table[zigzag[zz + 2]] = (val >> 16) & 0xFF;
            table[zigzag[zz + 3]] = (val >> 24) & 0xFF;
            /*table[zagzig[zz + 0]] = (val      ) & 0xFF;
            table[zagzig[zz + 1]] = (val >>  8) & 0xFF;
            table[zagzig[zz + 2]] = (val >> 16) & 0xFF;
            table[zagzig[zz + 3]] = (val >> 24) & 0xFF;*/
            /*table[zz + 0] = (val      ) & 0xFF;
            table[zz + 1] = (val >>  8) & 0xFF;
            table[zz + 2] = (val >> 16) & 0xFF;
            table[zz + 3] = (val >> 24) & 0xFF;*/
            
            counter++;
            
            break;
        }
        
        case 3: {
            // MDEC(3) - Set Scale Table
            
            // This command defines the IDCT scale matrix, which should be usually/always:
            /**
             * 5A82 5A82 5A82 5A82 5A82 5A82 5A82 5A82
             * 7D8A 6A6D 471C 18F8 E707 B8E3 9592 8275
             * 7641 30FB CF04 89BE 89BE CF04 30FB 7641
             * 6A6D E707 8275 B8E3 471C 7D8A 18F8 9592
             * 5A82 A57D A57D 5A82 5A82 A57D A57D 5A82
             * 471C 8275 18F8 6A6D 9592 E707 7D8A B8E3
             * 30FB 89BE 7641 CF04 CF04 7641 89BE 30FB
             * 18F8 B8E3 6A6D 8275 7D8A 9592 471C E707
             */
            
            /*for (int i = 0; i < 2; i++) {
                scaleTable[counter * 2 + i] = val >> (i * 16);
            }*/
            scaleTable[counter++] = static_cast<int16_t>(val & 0xFFFF);
            scaleTable[counter++] = static_cast<int16_t>((val >> 16) & 0xFFFF);
            
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
    counter = 0;
}

void MDEC::decodeBlocks() {
    auto src = input.begin();
    
    while (src != input.end()) {
        auto block = decodeMarcoBlocks(src);
        if (!block.has_value())
            continue;
        
        for (uint32_t px : block.value().data)
            output.push_back(px);
    }
}

std::optional<MDEC::DCTBlock> MDEC::decodeMarcoBlocks(std::vector<uint16_t>::iterator &src) {
    DCTBlock block;
    block.data.clear();
    block.data.resize(256);
    
    /*Cbblk.fill(0);
    Crblk.fill(0);
    Yblk0.fill(0);
    Yblk1.fill(0);
    Yblk2.fill(0);
    Yblk3.fill(0);*/
    
    if (command.DataOutputDepth > 1) {
        // 15bpp or 24bpp depth
        // decode_colored_macroblock
        if (!rl_decode_block(Crblk, src, colorQuantTable)) return std::nullopt; // ;Cr (low resolution)
        if (!rl_decode_block(Cbblk, src, colorQuantTable)) return std::nullopt; // ;Cb (low resolution)
        
        // ;Y1 (and upper-left  Cr,Cb)
        if (!rl_decode_block(Yblk0, src, luminanceQuantTable)) return std::nullopt;
        
        // ;Y2 (and upper-right Cr,Cb)
        if (!rl_decode_block(Yblk1, src, luminanceQuantTable)) return std::nullopt;
        
        // ;Y3 (and lower-left  Cr,Cb)
        if (!rl_decode_block(Yblk2, src, luminanceQuantTable)) return std::nullopt;
        
        // ;Y4 (and lower-right Cr,Cb)
        if (!rl_decode_block(Yblk3, src, luminanceQuantTable)) return std::nullopt;
        
        // TODO; I can't figure out why tf
        // swapping top-right and bottom-left,
        // gives correct results????? For 15bit at least 
        yuv_to_rgb(block, 0, 0,  Yblk0); // top-left
        yuv_to_rgb(block, 8, 0,  Yblk1); // top-right
        yuv_to_rgb(block, 0, 8,  Yblk2); // bottom-left
        yuv_to_rgb(block, 8, 8,  Yblk3); // bottom-right
    } else {
        // 4bpp or 8bpp depth
        // decode_monochrome_macroblock
        if (!rl_decode_block(Yblk0, src, luminanceQuantTable)) return std::nullopt;
        y_to_mono(block, Yblk0); // ;Y
        
        assert(false);
    }
    
    return block;
}

template <size_t bit_size, typename T = int16_t>
T extend_sign(uint64_t n) {
    static_assert(bit_size > 0 && bit_size < 63, "bit_size out of range");
    
    T mask = ((1LL << (bit_size - 1)) - 1);
    bool sign = (n & (1LL << (bit_size - 1))) != 0;
    
    T val = n & mask;
    if (sign) val |= ~mask;
    return val;
}

template <typename T>
T clamp(T v, T min, T max) {
    if (v > max) {
        return max;
    } else if (v < min) {
        return min;
    } else {
        return v;
    }
}

bool MDEC::rl_decode_block(std::array<int16_t, 64> &blk, std::vector<uint16_t>::iterator &src, const std::array<uint8_t, 64> &qt) {
    blk.fill(0);
    
    while (src != input.end() && *src == 0xFE00) {
        src++;
    }
    
    if (src == input.end()) return false;
    const DCT dct = *src++;
    int32_t cur = extend_sign<10>(dct.DC);
    int32_t val = cur * qt[0];
    
    for (int k = 0; k < 64;) {
        if (dct.Q == 0) {
            val = cur * 2;
        }
        
        val = clamp<int32_t>(val, -0x400, 0x3FF);
        
        if (dct.Q > 0) {
            blk.at(zagzig[k]) = val;
        } else {
            blk.at(k) = val;
        }
        
        if (src == input.end()) break;
        const RLE rle = *src++;
        cur = extend_sign<10>(rle.AC);
        k += rle.LEN + 1;
        
        val = (cur * qt[k] * dct.Q + 4) / 8;
    }
    
    real_idct_core(blk);
    
    return true;
}

/*bool MDEC::rl_decode_block(std::array<uint16_t, 64> &blk, std::vector<uint16_t>::iterator &src, const std::array<uint8_t, 64> &qt) {
    blk.fill(0);
    
    uint16_t n, k = 0;
    uint16_t q_scale = 0;
    uint16_t val = 0;
    
    auto signext10 = [](uint16_t x) -> int16_t {
        return (x & 0x200) ? (int16_t)(x | 0xFC00) : (int16_t)(x & 0x03FF);
    };
    
    // First value is DCT
    
    skip:
        if (src == input.end()) return false;
        
        // To avoid confusion
        n = *src;
        ++src;
        
        // Skip padding
        if (n == 0xFE00)
            goto skip;
        
        q_scale = (n >> 10) & 0x3F; // Contains scale value (not 'skip' value)
        
        // calc first value (without q_scale/8) (?)
        // https://fgiesen.wordpress.com/2024/10/23/zero-or-sign-extend/
        // https://www.geeksforgeeks.org/c/sign-extend-a-nine-bit-number-in-c/
        DCT dct = DCT(n);
        val = signext10(dct.DC);
        
        val = (val * qt[0]) / 8;
    
    // TODO; Uhh this is wrong
    while (true) {
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
            
            if (src == input.end()) return false;
            n = *src;
            ++src;
            
            // get next entry (or FE00h end code)
            if (n == 0xFE00)
                break;
            
            // Breh x2
            k = k + ((n >> 10) & 0x3F) + 1; // Skip zerofilled entries
            if (k >= 64)
                break;
            
            val = n & 0x3FF; // 10 bits
            if (val & 0x200) val |= ~0x3FF; // If signbit sent, extend
            
            val = (val * qt[k] * q_scale) / 8;
        
        // ;should end with n=FE00h (that sets k>63)
        if (k <= 63) {
            goto lop;
        }
    }
    
    real_idct_core(blk.data());
    
    // return (with "src" address advanced)
    //src++;
    
    return true;
}*/

void MDEC::fast_idct_core(int16_t *blk) {
    
}

void MDEC::real_idct_core(std::array<int16_t, 64> &blk) const {
    int64_t tmp[64];
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            
            for (int i = 0; i < 8; i++) {
                sum += scaleTable[i * 8 + y] * blk[x + i * 8];
            }
            
            tmp[x + y * 8] = sum;
        }
    }
    
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int64_t sum = 0;
            
            for (int i = 0; i < 8; i++) {
                sum += tmp[i + y * 8] * scaleTable[x + i * 8];
            }
            
            int round = (sum >> 31) & 1;
            blk[x + y * 8] = static_cast<uint16_t>((sum >> 32) + round);
        }
    }
}

struct RGB { uint8_t r, g, b; };
std::array<RGB, 256> p;
void MDEC::yuv_to_rgb(DCTBlock& block, uint16_t xx, uint16_t yy, std::array<int16_t, 64> &blk) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int px = xx + x;
            int py = yy + y;
            
            int16_t Y = blk[y * 8 + x];
            int uv_x = (xx + x) >> 1;
            int uv_y = (yy + y) >> 1;
            const int16_t Cb = Cbblk[uv_y * 8 + uv_x];
            const int16_t Cr = Crblk[uv_y * 8 + uv_x];
            
            /*int R = y + ((1436 * Cr) >> 10);
            int G = y - ((352 * Cb + 731 * Cr) >> 10);
            int B = y + ((1815 * Cb) >> 10);*/
            int R = Y + ((1436 * Cr + 512) >> 10);
            int G = Y - ((352 * Cb + 731 * Cr + 512) >> 10);
            int B = Y + ((1815 * Cb + 512) >> 10);
            
            R = std::clamp(R, -128, 127);
            G = std::clamp(G, -128, 127);
            B = std::clamp(B, -128, 127);
            
            /*if (command.DataOutputDepth == 2) {
                /*R = std::clamp(R + 128, 0, 255);
                G = std::clamp(G + 128, 0, 255);
                B = std::clamp(B + 128, 0, 255);#1#
                R = std::clamp(R, -128, 127);
                G = std::clamp(G, -128, 127);
                B = std::clamp(B, -128, 127);
            } else {
                R = std::clamp(R, -128, 127);
                G = std::clamp(G, -128, 127);
                B = std::clamp(B, -128, 127);
            }*/
            
            uint8_t r = uint8_t(R);
            uint8_t g = uint8_t(G);
            uint8_t b = uint8_t(B);
            
            if (!command.DataOutputSigned/* && command.DataOutputDepth == 3*/) {
                r ^= 0x80;
                g ^= 0x80;
                b ^= 0x80;
            }
            
            p[py * 16 + px] = { r, g, b };
            //p[py * 16 + px] = { b, g, r };
        }
    }
    
    if (command.DataOutputDepth == 3) {
        block.data.resize(256);
        
        for (int i = 0; i < 256; i++) {
            auto& p0 = p[i];
            
            uint16_t r5 = (p0.r >> 3) & 0x1F;
            uint16_t g5 = (p0.g >> 3) & 0x1F;
            uint16_t b5 = (p0.b >> 3) & 0x1F;
            
            block.data[i] =
                /*(0 << 15) |*/
                (b5 << 10) |
                (g5 << 5)  |
                (r5);
        }
        
        return;
    }
    
    /*for (int mb_y = 0; mb_y < 16; mb_y += 16) {
        for (int mb_x = 0; mb_x < 16; mb_x += 16) {
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x += 4) {
                    int i0 = (y * 16 + x + 0);
                    int i1 = (y * 16 + x + 1);
                    int i2 = (y * 16 + x + 2);
                    int i3 = (y * 16 + x + 3);
                    
                    uint32_t w0 = (p[i0].r << 16) | (p[i0].g << 8) | p[i0].b | ((p[i1].r & 0xFF) << 24);
                    uint32_t w1 = ((p[i1].g & 0xFF) << 0) | ((p[i1].b & 0xFF) << 8) | (p[i2].r << 16) | (p[i2].g << 24);
                    uint32_t w2 = (p[i2].b) | (p[i3].r << 8) | (p[i3].g << 16) | (p[i3].b << 24);
                    
                    block.data.push_back(w0);
                    block.data.push_back(w1);
                    block.data.push_back(w2);
                }
            }
        }
    }
    
    return;*/
    
    for (int i = 0; i < 256; i += 4) {
        auto& p0 = p[i + 0];
        auto& p1 = p[i + 1];
        auto& p2 = p[i + 2];
        auto& p3 = p[i + 3];
        
        uint32_t w0 =
            (p0.b) |
            (p0.g << 8) |
            (p0.r << 16) |
            (p1.b << 24);
        
        uint32_t w1 =
            (p1.g) |
            (p1.r << 8) |
            (p2.b << 16) |
            (p2.g << 24);
        
        uint32_t w2 =
            (p2.r) |
            (p3.b << 8) |
            (p3.g << 16) |
            (p3.r << 24);
        
        block.data.push_back(w0);
        block.data.push_back(w1);
        block.data.push_back(w2);
    }
}

/*void MDEC::yuv_to_rgb(DCTBlock& block, uint16_t xx, uint16_t yy, std::array<int16_t, 64> &blk) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int px = xx + x;
            int py = yy + y;
            
            int uv_x = px >> 1;
            int uv_y = py >> 1;
            
            int16_t Y  = blk[y * 8 + x];
            int16_t Cb = Cbblk[uv_y * 8 + uv_x];
            int16_t Cr = Crblk[uv_y * 8 + uv_x];
            
            int R = Y + ((1436 * Cr) >> 10);
            int G = Y - ((352 * Cb + 731 * Cr) >> 10);
            int B = Y + ((1815 * Cb) >> 10);
            
            uint8_t r = std::clamp(R + 128, 0, 255);
            uint8_t g = std::clamp(G + 128, 0, 255);
            uint8_t b = std::clamp(B + 128, 0, 255);
            
            if (command.DataOutputDepth == 3) {
                // 15-bit RGB555
                uint16_t rgb555 =
                    ((r >> 3) << 10) |
                    ((g >> 3) << 5)  |
                    (b >> 3);
                
                block.data[py * 16 + px] = rgb555;
            } else {
                // 24-bit RGB888 → two halfwords
                uint32_t rgb24 = (r << 16) | (g << 8) | b;
                
                uint16_t lo = rgb24 & 0xFFFF;   // RR GG
                uint16_t hi = rgb24 >> 16;      // BB 00
                
                block.data.push_back(lo);
                block.data.push_back(hi);
            }
        }
    }
}
*/

/*void MDEC::yuv_to_rgb(DCTBlock& block, uint16_t xx, uint16_t yy, std::array<uint16_t, 64> &blk) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            double R = Crblk[(x + xx) / 2 + ((y + yy) / 2) * 8];
            double B = Cbblk[(x + xx) / 2 + ((y + yy) / 2) * 8];
            double G = (B * -0.3437) + (R * -0.7143);
            
            R = (R * 1.402);
            B = (B * 1.772);
            
            const uint32_t Y = blk[x + y * 8];
            
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
            //uint16_t pixel = (B5 << 10) | (G5 << 5) | R5;
            block.data[(x + xx) + (y + yy) * 16] = RLE(pixel);
        }
    }
}
*/

void MDEC::y_to_mono(DCTBlock &block, std::array<int16_t, 64> &blk) {
    for (int i = 0; i < 64; i++) {
        int v = blk[i] & 0x1FF;
        if (v & 0x100) v |= ~0x1FF;
        v = std::clamp(v, -128, 127);
        
        if (!command.DataOutputSigned)
            v ^= 0x80;
        
        block.data[i] = (uint16_t)(v & 0xFF);
    }
}
