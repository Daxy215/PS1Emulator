#pragma once
#include <cstdint>

namespace Emulator {
    // https://psx-spx.consoledev.net/soundprocessingunitspu/#1f801daah-spu-control-register-spucnt
    struct Status {
        bool isWritingToFirstHalf; // 0 -> First, 1 -> Second
        bool dataTransferBusyFlag; // 0 -> Ready, 1 -> Busy
        bool dataTransferDmaRead;  // 0 -> No,    1 -> Yes
        bool dataTransferDmaWrite; // 0 -> No,    1 -> Yes
        bool dataDmaRequest;       // seems to be same as SPUCNT.Bit5
        bool irq9Flag;             // 0 -> No,    1 -> Interrupt Request 
        uint32_t mode; // 5-0 SPUMode;
    };
    
    class SPU {
    public:
        void load(uint32_t addr, uint32_t val);
        void stat(uint32_t val);

    public:
        Status status;
    };
}