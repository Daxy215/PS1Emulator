#include "VRAM.h"

#include <iostream>

#include "Gpu.h"
#include "Rendering/Renderer.h"

/**
 * Tried to make my own but it refused,
 * so I ended up giving up and using this guide:
 * 
 * https://www.reddit.com/r/EmuDev/comments/fmhtcn/article_the_ps1_gpu_texture_pipeline_and_how_to/
 */
Emulator::VRAM::VRAM(Gpu* gpu) : gpu(gpu) {
	uint32_t buffer_mode = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
	
	/*/* 16bit VRAM pixel buffer. #1#
	glGenBuffers(1, &pbo16);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo16);
	
	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512, nullptr, buffer_mode);	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	glGenTextures(1, &texture16);
	glBindTexture(GL_TEXTURE_2D, texture16);
	
	/* Set the texture wrapping parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	/* Set texture filtering parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* Allocate space on the GPU. #1#
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, 1024, 512, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, texture16);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo16);
	
	ptr16 = static_cast<uint16_t*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512, buffer_mode));
	
	/* 4bit VRAM pixel buffer. #1#
	glGenBuffers(1, &pbo4);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo4);
	
	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512 * 4, nullptr, buffer_mode);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	glGenTextures(1, &texture4);
	glBindTexture(GL_TEXTURE_2D, texture4);
	
	/* Set the texture wrapping parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	/* Set texture filtering parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* Allocate space on the GPU. #1#
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 4096, 512, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, texture4);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo4);
	
	ptr4 = static_cast<uint8_t*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512 * 4, buffer_mode));
	
    /* 8bit VRAM pixel buffer. #1#
	glGenBuffers(1, &pbo8);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo8);
	
	glBufferStorage(GL_PIXEL_UNPACK_BUFFER, 1024 * 512 * 2, nullptr, buffer_mode);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	glGenTextures(1, &texture8);
	glBindTexture(GL_TEXTURE_2D, texture8);
	
	/* Set the texture wrapping parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	/* Set texture filtering parameters. #1#
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	/* Allocate space on the GPU. #1#
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 2048, 512, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, texture8);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo8);
	
	ptr8 = static_cast<uint8_t*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 1024 * 512 * 2, buffer_mode));*/
	
	ptr16 = new uint16_t[1024 * 512];
	ptr8 = new uint8_t[2048 * 512 * 4];
	ptr4 = new uint8_t[4096 * 512 * 4];
	
	std::fill(ptr16, ptr16 + (1024 * 512), 0);
	std::fill(ptr8, ptr8 + (1024 * 512 * 2), 0);
	std::fill(ptr4, ptr4 + (1024 * 512 * 4), ((31 << 11) | (63 << 5) | (0)));
	
	glGenTextures(1, &texture16);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture16);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, ptr16);
	
	/*glGenTextures(1, &texture8);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture8);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr8);
	*/
	
	/*glGenTextures(1, &texture4);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture4);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4096, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr4);
	*/
	
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
	/* Upload 16bit texture. */
	/*glBindTexture(GL_TEXTURE_2D, texture16);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo16);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 512, GL_RED, GL_UNSIGNED_BYTE, 0);
	
	/* Upload 4bit texture. */
	/*glBindTexture(GL_TEXTURE_2D, texture4);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo4);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4096, 512, GL_RED, GL_UNSIGNED_BYTE, 0);*/
	
	/* Upload 8bit texture. */
	/*glBindTexture(GL_TEXTURE_2D, texture8);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo8);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2048, 512, GL_RGB, GL_UNSIGNED_BYTE, 0);*/
	
	glBindTexture(GL_TEXTURE_2D, texture16);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 512, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, ptr16);
	
	/*
	glBindTexture(GL_TEXTURE_2D, texture8);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr4);
	*/
	
	/*glBindTexture(GL_TEXTURE_2D, texture4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4096, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, ptr4);
	*/
	
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::cerr << "OpenGL error after glTexSubImage2D: " << error << '\n';
	}
}

void Emulator::VRAM::drawPixel(uint32_t pixel) {
    if(!gpu->preserveMaskedPixels || (getPixel(transferData.x, transferData.y) >> 24) == 0) {
        setPixel(transferData.x & 0x3FF, transferData.y & 0x1FF, pixel);
    }
    
    stepTransfer();
}

void Emulator::VRAM::setPixel(uint32_t x, uint32_t y, uint32_t color) {
    size_t index = y * MAX_WIDTH + x;
	
	/* Write data as 16bit. */
	/*ptr16[index] = (static_cast<uint16_t>(color));*/
	
	/* Write data as 8bit. */
	/*ptr8[index * 2 + 0] = static_cast<uint8_t>((color));
	ptr8[index * 2 + 1] = static_cast<uint8_t>((color) >> 8);*/
	
	/* Write data as 4bit. */
	/*ptr4[index * 4 + 0] = static_cast<uint8_t>((color) >> 0)  & 0xF;
	ptr4[index * 4 + 1] = static_cast<uint8_t>((color) >> 4)  & 0xF;
	ptr4[index * 4 + 2] = static_cast<uint8_t>((color) >> 8)  & 0xF;
	ptr4[index * 4 + 3] = static_cast<uint8_t>((color) >> 12) & 0xF;/*
	
	/* Write data as 16bit. */
	ptr16[index] = RGB555_to_RGB565(static_cast<uint16_t>(color));
	
	/* Write data as 8bit. */
	ptr8[index * 2 + 0] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color)));
	ptr8[index * 2 + 1] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color) >> 8));
	
	/* Write data as 4bit. */
	ptr4[index * 4 + 0] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color))) & 0xF;
	ptr4[index * 4 + 1] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color)) >> 4) & 0xF;
	ptr4[index * 4 + 2] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color)) >> 8) & 0xF;
	ptr4[index * 4 + 3] = static_cast<uint8_t>(RGB555_to_RGB565(static_cast<uint16_t>(color)) >> 12) & 0xF;
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

uint16_t Emulator::VRAM::RGB555_to_RGB565(uint16_t color) {
	uint16_t red = (color & 0x7C00) >> 10;
	uint16_t green = (color & 0x03E0) >> 5;
	uint16_t blue = color & 0x001F;
    
	uint16_t red565 = red;
	uint16_t green565 = (green << 1);
	uint16_t blue565 = blue;
	
	return (red565 << 11) | (green565 << 5) | blue565;
}
