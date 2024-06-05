#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

// Pre-channel Data
//TODO; Make this a class
// DMA transfer directon
enum Direction {
    FromRam,
    ToRam,
};

// DMA transfer step
enum Step {
    Decrement,
    Increment
};

// DMA transfer synchronization mode
enum Sync {
    // Transfer starts when the CPU writes to the Trigger bit,
    // and transfers everything at once
    Manual,

    // Sync blocks to DMA requests
    Request,

    // Used to transfer GPU command lists
    LinkedList
};

// The 7 DMA ports
enum Port {
    // Macroblock decoder input
    MdecIn = 0,

    // Macroblock decoder output
    MdecOut = 1,

    // Graphics Processing Unit
    Gpu = 2,

    // CD-ROM drive
    CdRom = 3,

    // Sound Processing Unity
    Spu = 4,

    // Extension port
    Pio = 5,

    // Used to clear the ordering table
    Otc = 6
};

struct PortC {
    static Port fromIndex(uint32_t index) {
        switch (index) {
        case 0: return MdecIn;
        case 1: return MdecOut;
        case 2: return Gpu;
        case 3: return CdRom;
        case 4: return Spu;
        case 5: return Pio;
        case 6: return Otc;
        default: throw std::runtime_error("Invalid port; " + index);
        }
    }
};

// Per-channel data
struct Channel {
    uint32_t control() {
        uint32_t r = 0;
        
        r |= (static_cast<uint32_t>(direction)) << 0;
        r |= (static_cast<uint32_t>(step)) << 1;
        r |= (static_cast<uint32_t>(chop)) << 8;
        r |= (static_cast<uint32_t>(sync)) << 9;
        r |= (static_cast<uint32_t>(chopDmaSz)) << 16;
        r |= (static_cast<uint32_t>(chopCpuSz)) << 20;
        r |= (static_cast<uint32_t>(enable)) << 24;
        r |= (static_cast<uint32_t>(trigger)) << 28;
        r |= (static_cast<uint32_t>(dummy)) << 29;
        
        return r;
    }
    
    void setControl(uint32_t val) {
        direction = ((val & 1) != 0) ? FromRam : ToRam; 
        step      = (((val >> 1) & 1) != 0) ? Decrement : Increment;
        
        chop = ((val >> 8) & 1) != 0;
        
        switch ((val >> 9) & 3) {
        case 0:
            sync = Manual;
            break;
        case 1:
            sync = Request;
            break;
        case 2:
            sync = LinkedList;
            break;
        default:
            throw std::runtime_error("Unknown DMA sync mode; " + ((val >> 9) & 3));
            
            break;
        }
        
        chopDmaSz = static_cast<uint8_t>(((val >> 16) & 7));
        chopCpuSz = static_cast<uint8_t>(((val >> 20) & 7));
        
        enable = ((val >> 24) & 1) != 0;
        this->trigger = ((val >> 28) & 1)!= 0;
        
        dummy = static_cast<uint8_t>((val >> 29) & 3);
    }

    // Retrieve value of the Block Control register
    uint32_t blockControl() {
        uint32_t bs = static_cast<uint32_t>(blockSize);
        uint32_t bc = static_cast<uint32_t>(blockCount);
        
        return (bc << 16) | bs;
    }
    
    // Set value of the Block Control register
    void setBlockControl(uint32_t val) {
        blockSize = static_cast<uint16_t>(val);
        blockCount = static_cast<uint16_t>((val >> 16));
    }
    
    void setBase(uint32_t val) {
        base = val & 0xFFFFFF;
    }
    
    // Return true if the channel has been started
    bool active() {
        // In manual sync mode the CPU must set the "trigger",
        // bit to start the transfer
        bool trigger = false;
        
        switch (sync) {
        case Manual:
            trigger = this->trigger;
            break;
        case Request:
            trigger = true;
            break;
        case LinkedList:
            trigger = true;
            break;
        }
        
        return enable & trigger;
    }

    // Return the DMA transfer size in bytes or None for linked list mode
    std::optional<uint32_t> transferSize() {
        uint32_t bs = static_cast<uint32_t>(blockSize);
        uint32_t bc = static_cast<uint32_t>(blockCount);

        switch (sync) {
        case Manual:
            // For manual mode only the block size is used
            return bs;
            break;
        case Request:
            // In DMA request mode we must transfer 'bc' blocks
            return (bc * bs);
            break;
        case LinkedList:
            // In linked list mode the size is not known ahead of
            // time: We stop when we encounter the "end of list"
            // marker (0xffffff)
            return std::nullopt;
            break;
        }
    }
    
    // Set the channel status to "completed" state
    void done() {
        enable = false;
        trigger = false;

        // XXX need to set the correct value for the other fields,
        // (in particular interrupts)
        
    }

    bool enable = false;
    
    // Used to start the DMA transfer whe 'sync' is 'Manual'
    bool trigger = false;
    
    // If the DMA 'chops' the transfer and lets the CPU run in the gaps
    bool chop = false;
    
    // Chopping DMA window size (log2 number of words)
    uint8_t chopDmaSz = 0;
    
    // Chopping CPU window size (lgo2 number of cycles)
    uint8_t chopCpuSz = 0;
    
    // Unknown 2 RW bits in configuration register
    uint8_t dummy = 0;

    // Size of a block in words
    uint16_t blockSize = 0;

    // Block count, Only used when 'sync' is 'Request'
    uint16_t blockCount = 0;
    
    // DMA start address
    uint32_t base = 0;
    
    Direction direction = ToRam;
    Step step = Increment;
    Sync sync = Manual;
};

class Dma {
public:
    Dma() {}
    
    // Helper functions
    // Retrieve the value of the interrupt register
    uint32_t interrupt();

    // Set the value of the interrupt register
    void setInterrupt(uint32_t val);
    
    // Returns the status of the DMA interrupt
    bool irq();
    
    void setControl(uint32_t val);

    // Returns a reference to a channel by the port number
    //Channel& getChannel(Port port) const { return channels[static_cast<size_t>(port)]; }
    
    // Returns a reference to a channel by port number
    Channel& getChannel(Port port) { return channels[static_cast<size_t>(port)]; }
public:
    // master IRQ enable
    bool irqEn = false;
    
    // IRQ enable for individual channels
    uint8_t channelIrqEn = 0;

    // IRQ flags for individual channels
    uint8_t channelIraqFlags = 0;
    
    // When set the interrupt is active unconditionally (even if 'irq_en' is false)
    bool forceIrq = false;
    
    // Bits [0:5] of the interrupt registers are RW but I don't know,
    // what they're supposed to do so I just store them and send them,
    // back untouched on reads
    uint8_t irqDummy = 0;
    
    // Rest value taken from the Nocash PSX spec
    uint32_t control = 0x07654321;
    
    // The 7 channel instances
    Channel channels[7];
};
