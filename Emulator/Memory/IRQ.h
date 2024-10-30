#pragma once

// TODO; This is wrong
enum class Interrupt {
	VBlank = 0,
	CdRom = 2,
	Dma = 3,
	Timer0 = 4,
	Timer1 = 5,
	Timer2 = 6,
	PadMemCard = 7
};

class IRQ {
public:
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
	
	static void trigger(Interrupt interrupt) {
		status |= 1 << static_cast<uint16_t>(static_cast<int32_t>(interrupt));
	}
	
public:
	// Im to lazy
	inline static uint16_t status = 0;
	
	uint16_t mask;
};

