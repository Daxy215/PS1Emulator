#include "VRAM.h"

//#include <GLFW/glfw3.h>

#include <iostream>

#include "Gpu.h"
#include "Rendering/Renderer.h"

Emulator::VRAM::VRAM(Gpu* gpu) : gpu(gpu) {
	ptr16 = new uint16_t[1024 * 512];
	/*ptr8  = new uint8_t[1024 * 512 * 2];
	ptr4  = new uint8_t[1024 * 512 * 4];*/
	
	for(int x = 0; x < 1024; x++) {
		for(int y = 0; y < 512; y++) {
			// Yellow = ((31 << 11) | (63 << 5) | (0)) = 0xFFE0
			setPixel(x, y, ((255 << 11) | (0 << 5) | (0)));
		}
	}
	
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, &ptr16);
	
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glTexSubImage2D: " << error << '\n';
	}
}

void Emulator::VRAM::store(uint32_t val) {
    // TODO; Apply dithering
    uint32_t pixel1 = (val >> 16);
    uint32_t pixel0 = (val & 0xFFFF);
    
    pixel0 |= (gpu->forceSetMaskBit << 15);
    pixel1 |= (gpu->forceSetMaskBit << 15);
    
    // draw pixel 0
    drawPixel(pixel0);
    
    // draw pixel 1
    drawPixel(pixel1);
}

void Emulator::VRAM::beginTransfer(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t imgSize) {
    transferData = {x, y, width, height, imgSize};
    /*printf("Started drawing at x=%d y=%d - w=%d h=%d\n", x, y, width, height);
    std::cerr << "";*/
}

void Emulator::VRAM::stepTransfer() {
    if(++transferData.x == transferData.originX + transferData.width) {
        transferData.x -= transferData.width;
        transferData.y++;
    }
}

void Emulator::VRAM::endTransfer() {
	//glActiveTexture(GL_TEXTURE0);
	/*glBindTexture(GL_TEXTURE_2D, textureID);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, &ptr16);
	
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glTexSubImage2D: " << error << '\n';
	}*/
}

void Emulator::VRAM::drawPixel(uint32_t pixel) {
    // TODO; Check if this is needed
    //if(!gpu->preserveMaskedPixels || (getPixelRGB888(transferData.x, transferData.y) >> 24) == 0) {
        setPixel(transferData.x & 0x3FF, transferData.y & 0x1FF, pixel);
    //}
    
    stepTransfer();
}

void Emulator::VRAM::setPixel(uint32_t x, uint32_t y, uint32_t color) {
    size_t index = (y * MAX_WIDTH) + x;
	
	ptr16[index] = static_cast<uint16_t>(color);
	
	/* Write data as 8bit. */
	/*ptr8[index * 2 + 0] = static_cast<uint8_t>(color);
	ptr8[index * 2 + 1] = static_cast<uint8_t>(color >> 8);
	
	/* Write data as 4bit. #1#
	ptr4[index * 4 + 0] = static_cast<uint8_t>(color) & 0xF;
	ptr4[index * 4 + 1] = static_cast<uint8_t>(color >> 4) & 0xF;
	ptr4[index * 4 + 2] = static_cast<uint8_t>(color >> 8) & 0xF;
	ptr4[index * 4 + 3] = static_cast<uint8_t>(color >> 12) & 0xF;*/
}

uint16_t Emulator::VRAM::getPixel(uint32_t x, uint32_t y) const {
    x %= MAX_WIDTH;
    y %= MAX_HEIGHT;
    
    size_t index = y * MAX_WIDTH + x;
    return ptr16[index];
}

uint16_t Emulator::VRAM::getPixel4(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY) {
    uint16_t texel = getPixel(pageX + x / 4, pageY + y);
    uint32_t index = (texel >> (x % 4) * 4) & 0xF;
    return getPixel(clutX + index, clutY);
}

uint16_t Emulator::VRAM::getPixel8(uint32_t x, uint32_t y, uint32_t clutX, uint32_t clutY, uint32_t pageX, uint32_t pageY) {
    uint16_t texel = getPixel(pageX + x / 2, pageY + y);
    uint32_t index = (texel >> (x % 2) * 8) & 0xFF;
    return getPixel(clutX + index, clutY);
}

uint16_t Emulator::VRAM::getPixel16(uint32_t x, uint32_t y, uint32_t pageX, uint32_t pageY) {
    return getPixel(x + pageX, y + pageY);
}
