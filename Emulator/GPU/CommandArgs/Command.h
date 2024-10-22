#pragma once
#include <cstdint>

/**
 * This is a static class that'll be,
 * used for the arguments for the GP0,
 * commands, such as Polygon, Rectangle, etc.
 */
namespace Emulator {
	namespace Commands {
		// https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gpu-render-polygon-commands
		union Polygon {
			struct {
				bool isRawTextured : 1;
				bool isTransparent : 1;
				bool isTextured : 1;
				bool isQuad : 1;
				bool isGouraud : 1;
				
				// Renaming 3 bits out of 8 bits
				uint8_t _ : 3;
			};
			
			Polygon(uint8_t command) : _command(command) {}
			
			int getLength() const {
				int size = isQuad ? 4 : 3;
				if (isTextured) size *= 2;
				if (isGouraud) size += (isQuad ? 4 : 3);
				
				return size;
			}
			
			int getVerticesCount() {
				return isQuad ? 4 : 3;
			}
			
			uint8_t _command;
		};
		
		//class Command {
		//public:
            inline static Polygon polygon = Polygon(0);
		//};
	}
}