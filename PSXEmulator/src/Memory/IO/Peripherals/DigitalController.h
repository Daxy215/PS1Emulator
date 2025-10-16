#pragma once

#include <queue>
#include <cstdint>

// TODO; I suppose make this as a master class?
// Controller types: https://psx-spx.consoledev.net/controllersandmemorycards/#controller-id-halfword-number-0

class DigitalController {
public:
    enum Mode {
        Idle,
        Transferring,
        Connected
    };
    
    // TODO; Move to an abstract class
    // https://psx-spx.consoledev.net/controllersandmemorycards/#controllers-standard-digitalanalog-controllers
    union ControllerInput {
        struct {
            uint16_t Select   : 1;
            
            // Digital controller doesn't have,
            // joysticks, so those [L3, R3] aren't used
            uint16_t L3       : 1;
            uint16_t R3       : 1;
			
            uint16_t Start    : 1;
            uint16_t Up       : 1;
            uint16_t Right    : 1;
            uint16_t Down     : 1;
            uint16_t Left     : 1;
            uint16_t L2       : 1;
            uint16_t R2       : 1;
            uint16_t L1       : 1;
            uint16_t R1       : 1;
            uint16_t Triangle : 1;
            uint16_t Circle   : 1;
            uint16_t Cross    : 1;
            uint16_t Square   : 1;
        };
        
        uint16_t _reg;
        uint8_t _byte[2];
        
        ControllerInput(uint16_t reg) : _reg(reg) {};
    };
    
public:
    DigitalController();
    
    // TODO; Not sure if this is needed?
    void step(uint32_t cycles);
    
    uint16_t load(uint32_t val);
    void reset();
    
private:
    // TODO; Temp
    //   5A41h=Digital Pad         (or analog pad/stick in digital mode; LED=Off)
    const uint16_t TYPE = 0x5A41;
    
    //uint16_t _buttons = 0xFFFF;
    
public:
    bool _interrupt = false;
    
public:
    Mode _mode = Idle;
    
public:
    ControllerInput _input;
    
private:
    std::queue<uint8_t> data;
    
};