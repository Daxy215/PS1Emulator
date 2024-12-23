#include "Rasterizer.h"

#include <algorithm>

#include "D:/libs/glm/glm/gtc/type_ptr.hpp"

Emulator::Rasterizer::Rasterizer(Gpu& gpu) : gpu(gpu) {
	
}

void Emulator::Rasterizer::drawTriangle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	//drawLine(positions[0], positions[1], colors[0], colors[1]);
	//drawLine(positions[1], positions[2], colors[1], colors[2]);
	//drawLine(positions[2], positions[0], colors[2], colors[0]);
	
	if(positions[0].x == 658) {
		printf("");
	}
	
	fillTrinagle(positions, colors, uvs, attributes);
}

void Emulator::Rasterizer::drawQuad(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	// [2, 3, 0]
	Gpu::Position tr1[] = { positions[2], positions[3], positions[0] };
	//drawTriangle(tr1, {}, {}, attributes);
	
	// [3, 0, 1]
	Gpu::Position tr2[] = { positions[3], positions[0], positions[1] };
	//drawTriangle(tr2, {}, {}, attributes);
}

void Emulator::Rasterizer::drawLine(const Emulator::Gpu::Position& p1, const Emulator::Gpu::Position& p2, const Emulator::Gpu::Color& c1, const Emulator::Gpu::Color& c2) const {
	int dx = abs(p2.x - p1.x);
	int dy = abs(p2.y - p1.y);
	int sx = p1.x < p2.x ? 1 : -1;
	int sy = p1.y < p2.y ? 1 : -1;
	int err = dx - dy;
    
	int x = p1.x;
	int y = p1.y;
    
	while (true) {
		//auto color = glm::mix(c1, c2, );
		gpu.vram->writePixel(x, y, c1.toU32());
        
		if (x == p2.x && y == p2.y) break;
		
		int e2 = err * 2;
		if (e2 > -dy) { err -= dy; x += sx; }
		if (e2 < dx) { err += dx; y += sy; }
	}
}

void Emulator::Rasterizer::fillTrinagle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	// Sort the points by Y-coordinate (ascending)
	Emulator::Gpu::Position vertices[3] = { positions[0], positions[1], positions[2] };
	std::sort(vertices, vertices + 3, [](const Emulator::Gpu::Position& a, Emulator::Gpu::Position& b) {
		return a.y < b.y;
	});
    
	// Get the sorted points
	const Emulator::Gpu::Position& v0 = vertices[0];
	const Emulator::Gpu::Position& v1 = vertices[1];
	const Emulator::Gpu::Position& v2 = vertices[2];
	
	auto notZero = [this](float x) {
		return (x == 0) ? 1 : x;
	};
	
	// Calculate the edge slopes
	float slope01 = float(notZero(v1.x - v0.x)) / float(notZero(v1.y - v0.y));
	float slope02 = float(notZero(v2.x - v0.x)) / float(notZero(v2.y - v0.y));
	float slope12 = float(notZero(v2.x - v1.x)) / float(notZero(v2.y - v1.y));
	
	// Scanline filling
	for (int y = v0.y; y <= v2.y; ++y) {
		// Calculate x-coordinates for the left and right edges of the triangle at the current scanline
		int xLeft, xRight;
		
		if (y < v1.y) {
			// Interpolate along the line from v0 to v2 for the left side
			xLeft = v0.x + (y - v0.y) * slope02;
			// Interpolate along the line from v0 to v1 for the right side
			xRight = v0.x + (y - v0.y) * slope01;
		} else {
			// Interpolate along the line from v1 to v2 for the left side
			xLeft = v1.x + (y - v1.y) * slope12;
			// Interpolate along the line from v0 to v2 for the right side
			xRight = v0.x + (y - v0.y) * slope02;
		}
		
		// TODO;
		// Ik I shouldn't do this but im lazy
		#define WIDTH  1024
		#define HEIGHT 512
        
		// Ensure xLeft is always less than xRight
		if (xLeft > xRight) std::swap(xLeft, xRight);
        
		// Fill between the two intersection points
		for (int x = xLeft; x <= xRight; ++x) {
			gpu.vram->writePixel(x, y, colors[0].toU32());
		}
	}	
}
