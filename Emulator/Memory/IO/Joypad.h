#pragma once

#include <queue>

enum Buttons {
    BUTTON_CROSS,
    BUTTON_CIRCLE,
    BUTTON_TRIANGLE,
    BUTTON_SQUARE,
    BUTTON_L1,
    BUTTON_R1,
    BUTTON_L2,
    BUTTON_R2,
    BUTTON_START,
    BUTTON_SELECT,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
    BUTTON_TOTAL
};

class ControllerState {
public:
    ControllerState() = default;
    
    /*ControllerState(SDL_GameController& controller)
        : buttons{}, axisLeftX(0), axisLeftY(0), axisRightX(0), axisRightY(0), _controller(&controller) {
        
    }
    
    void free() {
        // TODO;
        SDL_GameControllerClose(_controller);
        _controller = nullptr;
    }*/
    
public:
    bool buttons[BUTTON_TOTAL];
    
    int16_t axisLeftX;
    int16_t axisLeftY;
    int16_t axisRightX;
    int16_t axisRightY;
    
    //SDL_GameController* _controller;
};

class Joypad {
public:
    Joypad();
    
    void update();
    void updateConnectedControllers();
    
    uint32_t load(uint32_t offset);
    void store(uint32_t offset, uint32_t val);
    
    /*void handleButtonEvent(SDL_ControllerButtonEvent &event);
    void handleAxisEvent(SDL_ControllerAxisEvent &event);*/
	
	void attachController(int slot,  const ControllerState& controller);
    void detachController(int slot);
    
private:
    uint32_t txData = 0;
    uint32_t rxData = 0;
    
    std::queue<uint8_t> fifo;
    
private:
    ControllerState controllers[2];
};