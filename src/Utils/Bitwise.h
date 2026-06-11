#pragma once
#include <stdint.h>
#include <cmath>

namespace Emulator {
	namespace Utils {
		class Bitwise {
		public:
			static bool getBit(uint16_t val, int n) {
				return (val & (1 << n));
			}
		};
		
		static double mod(double x, double y) {
			return x - std::floor(x / y) * y;
		}
		
		static double modf(double x, double y) {
			const double epsilon = std::numeric_limits<double>::epsilon();
			double r = fmod(x, y);
			return (std::abs(r) < epsilon) ? 0.0 : r;
		}
	}
}
