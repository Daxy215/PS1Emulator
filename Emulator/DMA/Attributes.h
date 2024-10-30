
#include <stdexcept>
// Pre-channel Data
//TODO; Make this a class
// DMA transfer direction
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
