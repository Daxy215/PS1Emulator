#include "VRAM.h"
#include "Gpu.h"

#include <iostream>

Emulator::VRAM::VRAM(Gpu& gpu) : gpu(gpu) {
	tilesX = MAX_WIDTH / TILE_SIZE;
	tilesY = MAX_HEIGHT / TILE_SIZE;
	tileDirty.resize(tilesX * tilesY);
	
	glCreateTextures(GL_TEXTURE_2D, 1, &tex);
	glTextureStorage2D(tex, 1, /*GL_RGB5_A1*/GL_RGB5_A1, MAX_WIDTH, MAX_HEIGHT);
	
	glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glCreateBuffers(1, &pbo);
	
	GLsizeiptr size = MAX_WIDTH * MAX_HEIGHT * sizeof(uint16_t);
	
	glNamedBufferStorage(
		pbo,
		size,
		nullptr,
		GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT
	);
	
	gpuPtr = (uint16_t*)glMapNamedBufferRange(
		pbo,
		0,
		size,
		GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT
	);
	
	reset();
}

Emulator::VRAM::~VRAM() {
	glDeleteTextures(1, &tex);
	glDeleteBuffers(1, &pbo);
}

void Emulator::VRAM::endTransfer() {
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, MAX_WIDTH);
	
	for (uint32_t ty = 0; ty < tilesY; ty++) {
		for (uint32_t tx = 0; tx < tilesX; tx++) {
			uint32_t i = ty * tilesX + tx;
			if (!tileDirty[i]) continue;
			tileDirty[i] = 0;
			
			uint32_t x = tx * TILE_SIZE;
			uint32_t y = ty * TILE_SIZE;
			
			glTextureSubImage2D(
				tex,
				0,
				x,
				y,
				TILE_SIZE,
				TILE_SIZE,
				GL_RGBA,
				GL_UNSIGNED_SHORT_1_5_5_5_REV,
				(void*)((y * MAX_WIDTH + x) * sizeof(uint16_t))
			);
		}
	}
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void Emulator::VRAM::flushRegion(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, MAX_WIDTH);
	
	uint32_t srcY  = (MAX_HEIGHT - (y + h));
	uint32_t destY = (MAX_HEIGHT - y) - h;
	
	glTextureSubImage2D(
		tex,
		0,
		x, destY,
		w, h,
		GL_RGBA,
		GL_UNSIGNED_SHORT_1_5_5_5_REV,
		(void*)((srcY * MAX_WIDTH + x) * sizeof(uint16_t))
	);
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void Emulator::VRAM::writePixel(uint32_t x, uint32_t y, const uint16_t pixel) {
	const uint16_t pixel0 = (pixel & 0xFFFF);
	
	x &= (MAX_WIDTH - 1);
	y &= (MAX_HEIGHT - 1);
	
	if(gpu.preserveMaskedPixels && (getPixel(x, y) & 0x8000)) {
		return;
	}
	
	const uint16_t mask = (gpu.forceSetMaskBit << 15);
	setPixel(x, y, pixel0 | mask);
}

void Emulator::VRAM::setPixel(uint32_t x, uint32_t y, uint32_t color) {
	y = MAX_HEIGHT - y - 1;
	
	const uint16_t ps1 = color;
	
	const uint16_t b = (ps1 >> 10) & 0x1F;
	const uint16_t g = (ps1 >> 5)  & 0x1F;
	const uint16_t r = (ps1 >> 0)  & 0x1F;
	const uint16_t a = (ps1 >> 15) & 1;
	
	const uint16_t gl =
		(r << 0) |
		(g << 5) |
		(b << 10) |
		(a << 15);
	
	gpuPtr[y * MAX_WIDTH + x] = gl;
	markTile(x, y);
}

uint16_t Emulator::VRAM::getPixel(uint32_t x, uint32_t y) const {
	x %= MAX_WIDTH;
	y %= MAX_HEIGHT;
	
	y = MAX_HEIGHT - y - 1;
	
	return gpuPtr[y * MAX_WIDTH + x];
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
	std::fill(gpuPtr, gpuPtr + MAX_WIDTH * MAX_HEIGHT, 0);
	std::fill(tileDirty.begin(), tileDirty.end(), 1);
}

void Emulator::VRAM::markTile(uint32_t x, uint32_t y) {
	uint32_t tx = x / TILE_SIZE;
	uint32_t ty = y / TILE_SIZE;
	tileDirty[ty * tilesX + tx] = 1;	
}
