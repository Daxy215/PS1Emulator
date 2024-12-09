﻿#include "Channel.h"

//#include <stdexcept>

#include <stdexcept>
#include <string>

#include "Dma.h"
#include "../Memory/IRQ.h"

uint32_t Channel::control() {
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

void Channel::setControl(uint32_t val) {
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

uint32_t Channel::blockControl() {
	uint32_t bs = static_cast<uint32_t>(blockSize);
	uint32_t bc = static_cast<uint32_t>(blockCount);
	
	return (bc << 16) | bs;
}

void Channel::setBase(uint32_t val) {
	base = val & 0xFFFFFF;
}

bool Channel::active() {
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

std::optional<uint32_t> Channel::transferSize() {
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
	default:
		throw std::runtime_error("Unknown DMA sync mode; " + std::to_string(sync));
		
		break;
	}
}

void Channel::done(Dma dma, Port port) {
	enable = false;
	trigger = false;
	
	// XXX need to set the correct value for the other fields,
	// (in particular interrupts)
	auto prvIrq = dma.irq();
	
	auto en = dma.channelIrqEn & (1 << (static_cast<size_t>(port)));
	dma.channelIrqEn |= en;
	
	if(!prvIrq && dma.irq()) {
		IRQ::trigger(IRQ::Interrupt::Dma);
	}
}

