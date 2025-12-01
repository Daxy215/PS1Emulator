#include "Rasterizer.h"

#include <algorithm>

#include "glm/gtc/type_ptr.hpp"

Emulator::Rasterizer::Rasterizer(Gpu& gpu) : gpu(gpu) {
	
}

void Emulator::Rasterizer::drawTriangle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	//drawLine(positions[0], positions[1], colors[0], colors[1]);
	//drawLine(positions[1], positions[2], colors[1], colors[2]);
	//drawLine(positions[2], positions[0], colors[2], colors[0]);
		
	/*if(positions[0].x == 658) {
		printf("");
	}*/
	
	//fillTrinagle(positions, colors, uvs, attributes);
}

void Emulator::Rasterizer::drawQuad(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	return;
	
	// [2, 3, 0]
	Gpu::Position tr1[] = { positions[2], positions[3], positions[0] };
	drawTriangle(tr1, colors, {}, attributes);
	
	// [3, 0, 1]
	Gpu::Position tr2[] = { positions[3], positions[0], positions[1] };
	drawTriangle(tr2, colors, {}, attributes);
}

void Emulator::Rasterizer::drawRectangle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const {
	return;
	
	/*// [2, 3, 0]
	Gpu::Position tr1[] = { positions[2], positions[3], positions[0] };
	drawTriangle(tr1, colors, {}, attributes);
	
	// [3, 0, 1]
	Gpu::Position tr2[] = { positions[3], positions[0], positions[1] };
	drawTriangle(tr2, colors, {}, attributes);*/
	
	// [0, 1, 2]
	Gpu::Position tr1[] = { positions[0], positions[1], positions[2] };
	drawTriangle(tr1, colors, {}, attributes);
	
	// [0, 2, 3]
	Gpu::Position tr2[] = { positions[0], positions[2], positions[3] };
	drawTriangle(tr2, colors, {}, attributes);
}

void Emulator::Rasterizer::drawLine(const Emulator::Gpu::Position& p1, const Emulator::Gpu::Position& p2, const Emulator::Gpu::Color& c1, const Emulator::Gpu::Color& c2) const {
	return;
	
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
	std::sort(vertices, vertices + 3, [](const Emulator::Gpu::Position& a, const Emulator::Gpu::Position& b) {
		return a.y < b.y;
	});
	
	// Get the sorted points
	const Emulator::Gpu::Position& v0 = vertices[0];
	const Emulator::Gpu::Position& v1 = vertices[1];
	const Emulator::Gpu::Position& v2 = vertices[2];
	
	auto notZero = [this](float x) {
		return (x == 0) ? 1e-6f : x; // Use a very small epsilon value
	};
	
	auto lerp = [](float start, float end, float t) {
		return start + t * (end - start);
	};
	
	auto interpolateColor = [](const Emulator::Gpu::Color& c1, const Emulator::Gpu::Color& c2, float t) {
		return Emulator::Gpu::Color(
			static_cast<GLubyte>(c1.r + t * (c2.r - c1.r)),
			static_cast<GLubyte>(c1.g + t * (c2.g - c1.g)),
			static_cast<GLubyte>(c1.b + t * (c2.b - c1.b))
		);
	};
	
	// Scanline filling
	for (int y = v0.y; y < v2.y; y++) {
		// Calculate x-coordinates for the left and right edges of the triangle at the current scanline
		int xLeft, xRight;
		Emulator::Gpu::Color colorLeft, colorRight;
		
		if (y < v1.y) {
			// Interpolate along the edges v0-v1 and v0-v2
			float t01 = float(y - v0.y) / float(notZero(v1.y - v0.y));
			float t02 = float(y - v0.y) / float(notZero(v2.y - v0.y));
			
			xLeft = lerp(v0.x, v2.x, t02);
			if (attributes.usesColor())
				colorLeft = interpolateColor(colors[0], colors[2], t02);
			
			xRight = lerp(v0.x, v1.x, t01);
			if (attributes.usesColor())
				colorRight = interpolateColor(colors[0], colors[1], t01);
		} else {
			// Interpolate along edges v1-v2 and v0-v2
			float t12 = float(y - v1.y) / float(notZero(v2.y - v1.y));
			float t02 = float(y - v0.y) / float(notZero(v2.y - v0.y));
			
			xLeft = lerp(v1.x, v2.x, t12);
			if (attributes.usesColor())
				colorLeft = interpolateColor(colors[1], colors[2], t12);
			
			xRight = lerp(v0.x, v2.x, t02);
			if (attributes.usesColor())
				colorRight = interpolateColor(colors[0], colors[2], t02);
		}
		
		// Ensure xLeft is always less than xRight
		if (xLeft > xRight) {
			std::swap(xLeft, xRight);
			std::swap(colorLeft, colorRight);
		}
		
		// Fill between the two intersection points
		for (int x = xLeft; x < xRight; x++) {
			float t = float(x - xLeft) / float(xRight - xLeft);
			Emulator::Gpu::Color pixelColor = interpolateColor(colorLeft, colorRight, t);
			
			gpu.vram->writePixel(x, y, pixelColor.toU32());
		}
	}	
}
