#include "VRAM.h"

#include <iostream>

#include "Gpu.h"
#include "Rendering/Renderer.h"

Emulator::VRAM::VRAM(Gpu& gpu) : gpu(gpu) {
	ptr16 = new uint16_t[MAX_WIDTH * MAX_HEIGHT];
	std::fill(ptr16, ptr16 + (MAX_WIDTH * MAX_HEIGHT), 0);
	
	glGenTextures(1, &texture16);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture16);
	
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);
	
	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		std::cerr << "OpenGL error22 after glTexSubImage2D: " << error << '\n';
	}
}

void Emulator::VRAM::endTransfer() {
	glBindTexture(GL_TEXTURE_2D, texture16);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RGBA, 
					GL_UNSIGNED_SHORT_1_5_5_5_REV, ptr16);
    
	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glTexSubImage2D: " << error << '\n';
	}
}

/*void Emulator::VRAM::endTransfer() {
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glBindTexture(GL_TEXTURE_2D, texture16);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, ptr16);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, texture16);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, ptr16);
	
	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glTexSubImage2D: " << error << '\n';
	}
}*/

void Emulator::VRAM::writePixel(uint32_t x, uint32_t y, uint16_t pixel) {
	uint16_t pixel0 = (pixel & 0xFFFF);
	
	if(gpu.preserveMaskedPixels && (getPixel(x, y) & 0x8000)) {
		return;
	}
	
	uint16_t mask = (gpu.forceSetMaskBit << 15);
	setPixel(x, y, pixel0 | mask);
}

void Emulator::VRAM::setPixel(uint32_t x, uint32_t y, uint32_t color) {
	x %= MAX_WIDTH;
	y %= MAX_HEIGHT;
	
	size_t index = y * MAX_WIDTH + x;
	
	/* Write data as 16bit. */
	ptr16[index] = (static_cast<uint16_t>(color));
}

uint16_t Emulator::VRAM::getPixel(uint32_t x, uint32_t y) const {
	/*x %= MAX_WIDTH;
	y %= MAX_HEIGHT;*/

	const size_t index = y * MAX_WIDTH + x;
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

uint16_t Emulator::VRAM::RGB555_to_RGB565(uint16_t color) {
	uint16_t red = (color & 0x7C00) >> 10;
	uint16_t green = (color & 0x03E0) >> 5;
	uint16_t blue = color & 0x001F;
	
	uint16_t red565 = red;
	uint16_t green565 = (green << 1);
	uint16_t blue565 = blue;
	
	return (red565 << 11) | (green565 << 5) | blue565;
}

void Emulator::VRAM::reset() {
	//std::fill(ptr16, ptr16 + (1024 * 512), 0);
}