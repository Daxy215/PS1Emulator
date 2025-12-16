#ifndef MDEC_H
#define MDEC_H

#include <array>
#include <memory>
#include <optional>
#include <vector>

class MDEC {
    private:
        union Status {
            struct {
                uint32_t ParameterWordsRemaining : 16; // 15-0  Number of Parameter Words remaining minus 1  (FFFFh=None)  ;CMD.Bit0-15
                uint32_t CurrentBlock            : 3 ; // 18-16 Current Block (0..3=Y1..Y4, 4=Cr, 5=Cb) (or for mono: always 4=Y)
                uint32_t NotUsed                 : 4 ; // 22-19 Not used (seems to be always zero)
                uint32_t DataOutputBit15         : 1 ; // 23    Data Output Bit15  (0=Clear, 1=Set) (for 15bit depth only) ;CMD.25
                uint32_t DataOutputSigned        : 1 ; // 24    Data Output Signed (0=Unsigned, 1=Signed)                  ;CMD.26
                uint32_t DataOutputDepth         : 2 ; // 26-25 Data Output Depth  (0=4bit, 1=8bit, 2=24bit, 3=15bit)      ;CMD.28-27
                uint32_t DataOutRequest          : 1 ; // 27    Data-Out Request (set when DMA1 enabled and ready to send data)
                uint32_t DataInRequest           : 1 ; // 28    Data-In Request  (set when DMA0 enabled and ready to receive data)
                uint32_t CommandBusy             : 1 ; // 29    Command Busy  (0=Ready, 1=Busy receiving or processing parameters)
                uint32_t DataInFifoFull          : 1 ; // 30    Data-In Fifo Full   (0=No, 1=Full, or Last word received)
                uint32_t DataOutFifoEmpty        : 1 ; // 31    Data-Out Fifo Empty (0=No, 1=Empty)
            } __attribute__((__packed__));
            
            uint32_t reg = 0;
            
            Status() = default;
            explicit Status(const uint32_t reg) : reg(reg) {}
        };
        
        union Control {
            struct {
                uint32_t Unknown              : 29; // 28-0  Unknown/Not used - usually zero
                uint32_t EnableDataOutRequest : 1 ; // 29    Enable Data-Out Request (0=Disable, 1=Enable DMA1 and Status.bit27)
                uint32_t EnableDataInRequest  : 1 ; // 30    Enable Data-In Request  (0=Disable, 1=Enable DMA0 and Status.bit28)
                uint32_t ResetMDEC            : 1 ; // 31    Reset MDEC (0=No change, 1=Abort any command, and set status=80040000h)
            } __attribute__((__packed__));
            
            uint32_t reg = 0;
            
            Control() = default;
            explicit Control(const uint32_t reg) : reg(reg) {}
        };
        
        union DecodeCommand {
            struct {
                uint32_t NumberOfParameterWords : 16; // 15-0  Number of Parameter Words (size of compressed data)
                uint32_t NotUsed                : 9 ; // 24-16 Not used (should be zero)
                uint32_t DataOutputBit15        : 1 ; // 25    Data Output Bit15  (0=Clear, 1=Set) (for 15bit depth only) ;STAT.23
                uint32_t DataOutputSigned       : 1 ; // 26    Data Output Signed (0=Unsigned, 1=Signed)                  ;STAT.24
                uint32_t DataOutputDepth        : 2 ; // 28-27 Data Output Depth  (0=4bit, 1=8bit, 2=24bit, 3=15bit)      ;STAT.26-25
                uint32_t Op                     : 3 ; // 31-29 Command (1=decode_macroblock)
            } __attribute__((__packed__));
            
            uint32_t reg = 0;
            
            DecodeCommand() = default;
            explicit DecodeCommand(const uint32_t reg) : reg(reg) {}
        };
        
        union DCT {
            struct {
                uint16_t DC : 10; // 9-0   DC   Direct Current reference (10 bits, signed)
                uint16_t Q : 6 ; // 15-10 Q    Quantization factor (6 bits, unsigned)
            };
            
            uint16_t reg = 0;
            
            DCT() = default;
            explicit DCT(const uint16_t reg) : reg(reg) {}
        };
        
        union RLE {
            struct {
                uint16_t  AC  : 10; // 9-0   AC   Relative AC value (10 bits, signed)
                uint16_t LEN : 6 ; // 15-10 LEN  Number of zero AC values to be inserted (6 bits, unsigned)
            };
            
            uint16_t reg = 0;
            
            RLE() = default;
            explicit RLE(const uint16_t reg) : reg(reg) {}
        };
        
        struct DCTBlock {
            DCT dct;
            //std::array<uint16_t, 16*16> data;
            std::vector<uint32_t> data;
            uint16_t EOB{}; // Fixed to FE00h
            
            DCTBlock() = default;
        };
        
    public:
        MDEC();
        
    public:
        uint32_t load(uint32_t addr);
        void store(uint32_t addr, uint32_t val);
        
        void reset();
        
        [[nodiscard]] bool dataInRequest () const { return status.DataInRequest  &&  output.empty(); };
        [[nodiscard]] bool dataOutRequest() const { return status.DataOutRequest && !output.empty(); };
        
    private:
        void handleCommand();
        void handleCommandProcessing(uint32_t val);
        
    public:
        void                    decodeBlocks();
        std::optional<DCTBlock> decodeMarcoBlocks(std::vector<uint16_t>::iterator &src);
        
        bool rl_decode_block(std::array<int16_t, 64> &blk, std::vector<uint16_t>::iterator &src, const std::array<uint8_t, 64> &qt);
        
        void fast_idct_core(int16_t *blk);
        void real_idct_core(std::array<int16_t, 64>& blk) const;
        
        void yuv_to_rgb(DCTBlock& block, uint16_t xx, uint16_t yy, std::array<int16_t, 64> &blk);
        void y_to_mono(DCTBlock &block, std::array<int16_t, 64> &blk);
        
    private:
        bool color = false; // (0=Luminance only, 1=Luminance and Color)
        
        uint32_t paramCount = 0;
        uint32_t counter = 0;
        uint32_t outputIndex = 0;
        
    public:
        Status status = Status(0);
        Control control = Control(0);
        DecodeCommand command = DecodeCommand(0);
        
    private:
        // Uhh ig this is meant to be 2d, 8x8
        std::array<DCTBlock, 64> blocks;
        
    public:
        // Input from idk DMA or whatever
        std::vector<uint16_t> input;
        
        // Output data from algorithm
        std::vector<uint32_t> output;
        
        std::array<uint8_t, 64> luminanceQuantTable;
        std::array<uint8_t, 64> colorQuantTable;
        std::array<int16_t, 64> scaleTable;
        
        // scalefactor[0..7] = cos((0..7)*90'/8) ;for [1..7]: multiplied by sqrt(2)
        // 1.000000000, 1.387039845, 1.306562965, 1.175875602,
        // 1.000000000, 0.785694958, 0.541196100, 0.275899379
        std::array<double, 8> scaleFactor = {
            1.000000000, 1.387039845, 1.306562965, 1.175875602,
            1.000000000, 0.785694958, 0.541196100, 0.275899379
        };
        
        // Values obtained from https://psx-spx.consoledev.net/macroblockdecodermdec/#1f801824h-mdec1-mdec-status-register-r
        std::array<uint8_t, 64> zigzag = {
            0 ,1 ,5 ,6 ,14,15,27,28,
            2 ,4 ,7 ,13,16,26,29,42,
            3 ,8 ,12,17,25,30,41,43,
            9 ,11,18,24,31,40,44,53,
            10,19,23,32,39,45,52,54,
            20,22,33,38,46,51,55,60,
            21,34,37,47,50,56,59,61,
            35,36,48,49,57,58,62,63
        };
        
        std::array<double, 64> scaleZag;
        
        // reversed of zigzag table
        std::array<uint8_t, 64> zagzig;
        
        std::array<int16_t, 64> Crblk;
        std::array<int16_t, 64> Cbblk;
        std::array<int16_t, 64> Yblk0, Yblk1, Yblk2, Yblk3;
        //std::array<uint16_t, 64> iq_uv;
        //std::array<uint16_t, 64> iq_y;
};

#endif //MDEC_H