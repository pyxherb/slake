#include "fmod.h"

using namespace slake;
using namespace slake::flib;

float flib::fmodf(float n, float d) {
	if (n < d)
		return n;

	while (true) {
		float q = n / d;

		uint32_t *qInt = (uint32_t *)&q;
		int8_t exponent = (int8_t)(((int16_t)((*qInt & 0b01111111100000000000000000000000) >> 23)) - 127);

		if (exponent > 0) {
			if (exponent < 32) {
				n -= d * (1u << exponent);
			} else if (exponent < 64) {
				n -= d * (1ull << exponent);
			} else {
				// x = 2 ^ 63, y = 2 ^ (e - 63)
				float x = 1ull << 63, y = 1ull << (exponent - 63);
				n -= d * (x * y);
			}
		} else {
			if (n >= d)
				return n - d;
			else
				return n;
		}
	}

	return n;
}
