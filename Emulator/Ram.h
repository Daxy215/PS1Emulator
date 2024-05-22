#pragma once
#include <optional>
#include <vector>

class Ram {
public:
    Ram();

    // Load using little endian at offset
    uint32_t load32(uint32_t offset);
    uint16_t load16(uint16_t offset);
    uint8_t load8(uint8_t offset);

    // Store val at offset
    void store32(uint32_t offset, uint32_t val);
    void store16(uint32_t offset, uint16_t val);
    void store8(uint32_t offset, uint8_t val);

private:
    std::vector<uint8_t> data;
};
