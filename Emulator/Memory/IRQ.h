#pragma once

class IRQ {
public:
	enum Interrupt {
		VBlank = 0,
		GPU = 1, // Rarely used, triggered by GP0(1F)
		CDROM = 2,
		Dma = 3,
		Timer0 = 4, // Root Counter 0 (Sysclk or Dotclk)
		Timer1 = 5, // Root Counter 1 (Sysclk or H-blank)
		Timer2 = 6, // Root Counter 2 (Sysclk or Sysclk /8)
		PadMemCard = 7, // Byte Received Interrupt
		SIO = 8, // ??
		SPU = 9,
		Controller = 10, // Lightpen interrupt. Also shared by PIO and DTL cards
	};
	
	IRQ() = default;
	
	explicit operator bool() const {
		return status & mask;
	}
	
	[[nodiscard]] uint16_t getStatus() const {
		return status;
	}
	
	void acknowledge(uint16_t ack) noexcept {
		status &= ack;
	}
	
	[[nodiscard]] uint16_t getMask() const {
		return mask;
	}
	
	void setMask(uint16_t mask) {
		this->mask = mask;
	}
	
	static uint32_t active() {
		return (status & mask) != 0;
	}

	static void trigger(Interrupt interrupt) {
		//status |= (interrupt);
		//status |= static_cast<int32_t>(interrupt);
		status |= (1 << static_cast<uint16_t>((interrupt)));
	}
	
public:
	// Im to lazy
	inline static uint16_t status = 0;
	inline static uint16_t mask = 0;
};

