#pragma once
#include <cstdint>

namespace Emulator {
	namespace Utils {
		class Bitwise {
		public:
			static bool getBit(uint16_t val, int n) {
				return (val & (1 << n));
			}
		};
	}
}
