#pragma once
#include <stdint.h>
#include <vector>

namespace Emulator {
    // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801daah-spu-control-register-spucnt
    union Status {
        struct {
            uint16_t Current_SPU_Mode          : 6; // Bits 0-5: Current SPU Mode (delayed version of SPUCNT.Bit5-0)
            uint16_t IRQ9_Flag                 : 1; // Bit 6: IRQ9 Flag (0=No, 1=Interrupt Request)
            uint16_t DMA_Read_Write_Request    : 1; // Bit 7: Data Transfer DMA Read/Write Request (same as SPUCNT.Bit5)
            uint16_t DMA_Write_Request         : 1; // Bit 8: Data Transfer DMA Write Request (0=No, 1=Yes)
            uint16_t DMA_Read_Request          : 1; // Bit 9: Data Transfer DMA Read Request (0=No, 1=Yes)
            uint16_t Data_Transfer_Busy_Flag   : 1; // Bit 10: Data Transfer Busy Flag (0=Ready, 1=Busy)
            uint16_t Capture_Buffer_Half       : 1; // Bit 11: Writing to Capture Buffers (0=First, 1=Second half)
            uint16_t Unused                    : 4; // Bits 12-15: Unknown/Unused (usually zero)
        };
        
        uint32_t _reg;
        
        Status() = default;
        Status(uint32_t reg) : _reg() {
                
        }
    };
    
    // SPU Control Register
    union Spucnt {
        struct {
            uint16_t CD_Audio_Enable        : 1;  // 0     CD Audio Enable         (0=Off, 1=On) (for CD-DA and XA-ADPCM)
            uint16_t External_Audio_Enable  : 1;  // 1     External Audio Enable   (0=Off, 1=On)
            uint16_t CD_Audio_Reverb        : 1;  // 2     CD Audio Reverb         (0=Off, 1=On) (for CD-DA and XA-ADPCM)
            uint16_t External_Audio_Reverb  : 1;  // 3     External Audio Reverb   (0=Off, 1=On)
            uint16_t Sound_RAM_Transfer     : 2;  // 5-4   Sound RAM Transfer Mode (0=Stop, 1=ManualWrite, 2=DMAwrite, 3=DMAread)
            uint16_t IRQ9_Enable            : 1;  // 6     IRQ9 Enable (0=Disabled/Acknowledge, 1=Enabled; only when Bit15=1)
            uint16_t Reverb_Master_Enable   : 1;  // 7     Reverb Master Enable    (0=Disabled, 1=Enabled)
            uint16_t Noise_Freq_Step        : 2;  // 9-8   Noise Frequency Step    (0..03h = Step "4,5,6,7")
            uint16_t Noise_Freq_Shift       : 4;  // 13-10 Noise Frequency Shift   (0..0Fh = Low .. High Frequency)
            uint16_t Mute_SPU               : 1;  //  4    Mute SPU                (0=Mute, 1=Unmute)  (Don't care for CD Audio)
            uint16_t SPU_Enable             : 1;  // 15    SPU Enable              (0=Off, 1=On)       (Don't care for CD Audio)
        };
        
        uint32_t _reg;
        
        Spucnt() = default;
        Spucnt(uint32_t reg) : _reg(reg) {}
    };
    
    class SPU {
    public:
        SPU();
        
        uint32_t load(uint32_t addr, uint32_t val);
        void store(uint32_t addr, uint32_t val);
        
    private:
        void handleVoiceStore(uint32_t addr, uint32_t val);
        
        uint32_t handleVolumeLoad(uint32_t addr);
        void handleVolumeStore(uint32_t addr, uint32_t val);

        void handleFlagsStore(uint32_t addr, uint32_t val);
        
        uint32_t handleControlLoad(uint32_t addr);
        void handleControlStore(uint32_t addr, uint32_t val);
        
    private:
        // Flags
        struct Flags {
            uint32_t keyOff = 0;
        };
        
        // SPU ADPCM Pitch
        struct ADPCM {
            // Start Address (This gets copied to "current address" upon Key On)
            uint16_t startAddress = 0;
            
            // ADPCM Sample Rate
            uint16_t vxPitch = 0;
            
            // Pitch Modulation Enabled Flags
            uint32_t PMON = 0;
        };
        
        // SPU Noise Generator
        
        // Noise mode enable
        uint32_t NON = 0;
        
        // Reverb Registers
        uint16_t vLOUT = 0;
        uint16_t vROUT = 0;
        
        // Echo On
        uint32_t EON = 0;
        
        // SPU Volume and ADSR Generator
        struct ADSR {
            // 1F801C08h+N*10h - Voice 0..23 Attack/Decay/Sustain/Release (ADSR) (32bit)
            uint32_t adsr = 0;
            
            // 1F801C00h+N*10h - Voice 0..23 Volume Left
            uint32_t volLeft = 0;
            
            // 1F801C02h+N*10h - Voice 0..23 Volume Right
            uint32_t volRight = 0;
        };
        
        // 1F801D80h - Mainvolume left
        uint16_t mainValLeft = 0;
        
        // 1F801D82h - Mainvolume right
        uint16_t mainValRight = 0;
        
        // 1F801DB0h - CD Audio Input Volume (for normal CD-DA, and compressed XA-ADPCM)
        uint32_t cdInputVol = 0;
        
        // 1F801DB4h - External Audio Input Volume
        uint16_t exVolLeft = 0;
        uint16_t exVolRight = 0;
        
    private:
        uint64_t currentAddress = 0;
        
        static const uint8_t VOICE_COUNT = 24;
        
    private:
        Status status = {0};
        Spucnt spunct = {0};
        
        // Registers
        Flags flags[VOICE_COUNT];
        ADPCM adpcm[VOICE_COUNT];
        ADSR adsr[VOICE_COUNT];
        
        std::vector<uint32_t> buffer;
    };
}
