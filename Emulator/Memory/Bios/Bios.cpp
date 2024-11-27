#include "Bios.h"

#include <vector>

Bios::Bios(const std::string& path)  {
    std::ifstream file(path, std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }
    
    std::vector<uint8_t> content;
    
    // Get the file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size < 0) {
        throw std::runtime_error("Failed to determine file size");
    }
    
    // I'm actually speachless..
    // I was loading the bios in a wrong format.. ;-;
    content.resize(size);
    data.resize(size);
    
    if (!file.read(reinterpret_cast<char*>(content.data()), size)) {
        throw std::runtime_error("Failed to read file content");
    }
    
    std::copy(content.begin(), content.end(), data.begin());
    
    auto write = [&](uint32_t address, uint32_t opcode) {
        address &= data.size() - 1;
        
        for (int i = 0; i < 4; i++) {
            data[address + i] = (opcode >> (i * 8)) & 0xff;
        }
    };
    
    // Enable TTY
    write(0x6F0C, 0x24010001);
    write(0x6F14, 0xAF81A9C0);
    
    //data.insert(data.end(), std::istreambuf_iterator<char>(file), {});
    
    file.close();
    
    /*data.resize(BIOS_SIZE);
    file.read(reinterpret_cast<char*>(data.data()), BIOS_SIZE);
    
    // Convert big-endian to little-endian
    for (size_t i = 0; i < data.size(); i += 4) {
        std::swap(data[i], data[i + 3]);
        std::swap(data[i + 1], data[i + 2]);
    }*/
    
    if (data.size() != BIOS_SIZE) {
        throw std::runtime_error("Invalid BIOS size");
    }
}

/*uint32_t Bios::getLittleEndian(std::ifstream& file) {
    uint32_t val;
    char bytes[4];
    file.read(bytes, 4);
    
    // Reorder bytes for little endian
    val = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    
    return val;
}*/

/**
 * Offset here isn't a CPU address but rather,
 * an offset within the BIOS memory range.
 */
/*uint32_t Bios::load32(size_t offset) {
    /*uint32_t b0 = data[offset + 0];
    uint32_t b1 = data[offset + 1];
    uint32_t b2 = data[offset + 2];
    uint32_t b3 = data[offset + 3];
    
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);#1#
    
    // Same as above but it's better I suppose?
    // Idk I want to give myself something for wasting my time further
    
    uint32_t result = 0;
    
    for (int i = 0; i < 4; ++i) {
        result |= static_cast<uint32_t>(data[offset + i]) << ((3 - i) * 8);
    }
    
    return result;
}

uint8_t Bios::load8(size_t offset) {
    return data[offset];
}*/
