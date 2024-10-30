#pragma once
#include <vector>

class Sector {
public:
	Sector(size_t size) : pointer(0), size(size) {
		/**
		 * Sector contains;
		 * 12 bytes as a sync filed; Used for syncing to identify the beginning of the sector
		 * 4 bytes as the header; Contains the time to help locate data
		 * 8 bytes as the subheader; Used for error corection and data validation
		 * 2048 bytes as user data; This is accessable by the software, holds the executable data, assets or music
		 * 280 bytes used for error correction (ECC); Helps to correct minor read errors
		 * 4 bytes as the end of sector; Marks the end of the sector.
		 *
		 * Sector size 2352 bytes,
		 * with 2048 being user data,
		 * and 304 used for ECC or other.
		 */
		buffer.reserve(size);
	}
	
	void set(const std::vector<uint8_t>& data);
	
	uint8_t& load8();
	int16_t& load16();
	
	std::vector<uint8_t> read();
	std::vector<uint8_t> read(size_t size);
	
	bool isEmpty();
	void empty();

private:
	size_t size;
	size_t pointer;
	std::vector<uint8_t> buffer;

public:
	static const int RAW_BUFFER = 2352;
	static const int XA_BUFFER = RAW_BUFFER * 16;
	
};
