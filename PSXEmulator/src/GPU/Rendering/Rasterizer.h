#pragma once

#include "../Gpu.h"

namespace Emulator {
	class Rasterizer {
	public:
		Rasterizer(Gpu& gpu);
		
		void drawTriangle (const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const;
		void drawQuad     (const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const;
		void drawRectangle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], const Gpu::Attributes attributes) const;
		
	private:
		void drawLine    (const Emulator::Gpu::Position& p1, const Emulator::Gpu::Position& p2, const Emulator::Gpu::Color& c1, const Emulator::Gpu::Color& c2) const;
		void fillTrinagle(const Emulator::Gpu::Position positions[], const Emulator::Gpu::Color colors[], const Emulator::Gpu::UV uvs[], Gpu::Attributes attributes) const;
		
	private:
		Gpu& gpu;
	};
}
