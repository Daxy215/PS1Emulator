#include "Joypad.h"

#include <iostream>

Joypad::Joypad() : controllers{} {
	
}

void Joypad::update() {
	
}

void Joypad::updateConnectedControllers() {
	//TODO; I'm too lazy
	/*if(controllers[0] && controllers[1] != nullptr) {
		std::cerr << "Max numbers of controllers reached!\n";
		return;
	}
	*/
	
	// Get connected controllers
	int slot = 0;
	
	/*for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			SDL_GameController* controller = SDL_GameControllerOpen(i);

			// TODO; Handle disconnection
			if (controller) {
				std::cout << "Controller connected: " << SDL_GameControllerName(controller) << '\n';
				
				attachController(slot, {*controller});
				
				if(++slot >= 2) {
					std::cerr << "Max numbers of controllers reached!\n";
					return;
				}
				
				break;
			}
		}
	}*/
}

uint32_t Joypad::load(uint32_t offset) {
	if (offset == 0x1F801040) {
		// JOY_TX_DATA (R) - Read the next value in RX FIFO
		uint32_t value = 0xFFFFFFFF;
		
		return value;
	}
	
	return 0x1;
}

void Joypad::store(uint32_t offset, uint32_t val) {
	if (offset == 0x1F801040) {
		// JOY_TX_DATA (W) - Write the value to be transmitted
		
		txData = val;
		rxData = 0xFF;		
	}
}

/*
// TODO; Maybe use SDL_GameControllerGetButton instead?
void Joypad::handleButtonEvent(SDL_ControllerButtonEvent& event) {
	bool buttonPressed = event.state == SDL_PRESSED;
	
	// TODO; For now only use controller[0] if it exists
	ControllerState controllerState = controllers[0];
	
	if(!controllerState._controller)
		return;
	
	switch (event.button) {
	case SDL_CONTROLLER_BUTTON_A:
		controllerState.buttons[BUTTON_CROSS] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_B:
		controllerState.buttons[BUTTON_CIRCLE] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_X:
		controllerState.buttons[BUTTON_SQUARE] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		controllerState.buttons[BUTTON_TRIANGLE] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		controllerState.buttons[BUTTON_L1] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		controllerState.buttons[BUTTON_R1] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		controllerState.buttons[BUTTON_SELECT] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_START:
		controllerState.buttons[BUTTON_START] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		controllerState.buttons[DPAD_UP] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		controllerState.buttons[DPAD_DOWN] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		controllerState.buttons[DPAD_LEFT] = buttonPressed;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		controllerState.buttons[DPAD_RIGHT] = buttonPressed;
		break;
	}
}

void Joypad::handleAxisEvent(SDL_ControllerAxisEvent& event) {
	// TODO; For now only use controller[0] if it exists
	ControllerState controllerState = controllers[0];
	
	if(!controllerState._controller)
		return;
	
	switch (event.axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		controllerState.axisLeftX = event.value;
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		controllerState.axisLeftY = event.value;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		controllerState.axisRightX = event.value;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		controllerState.axisRightY = event.value;
		break;
	}
}
*/

void Joypad::attachController(int slot, const ControllerState& controller) {
	if (slot < 0 || slot >= 2)
		return;
	
	controllers[slot] = controller;
}

void Joypad::detachController(int slot) {
	if (slot < 0 || slot >= 2) return;
	
	//controllers[slot].free();
}
