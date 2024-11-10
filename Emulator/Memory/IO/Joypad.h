#pragma once

#include <queue>

// TODO; I suppose make this as a master class?
// Controller types: https://psx-spx.consoledev.net/controllersandmemorycards/#controller-id-halfword-number-0

class Joypad {
    enum Mode {
        Idle,
        Transfering,
        Connected
    };
    
public:
    Joypad();
    
    // TODO; Not sure if this is needed?
    void step(uint32_t cycles);
    
    uint16_t load(uint32_t val);
    void reset();

private:
    // TODO; Temp
    //   5A41h=Digital Pad         (or analog pad/stick in digital mode; LED=Off)
    const uint16_t TYPE = 0x5A41;
    
    uint16_t _buttons = 0;
    
public:
    bool _interrupt = false;
    
private:
    Mode _mode = Idle;
    
private:
    std::queue<uint8_t> data;
    
};