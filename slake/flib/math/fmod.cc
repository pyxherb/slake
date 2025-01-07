#include "fmod.h"
#include <cstdint>

using namespace slake;
using namespace slake::flib;

SLAKE_API float flib::fmodf(float n, float d) {
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
			if (n >= d) {
				n -= d;
			} else
				return n;
		}
	}
}

SLAKE_API double flib::fmod(double n, double d) {
	if (n < d)
		return n;

	while (true) {
		double q = n / d;

		uint64_t *qInt = (uint64_t *)&q;
		int16_t exponent = (((int16_t)((*qInt & 0b0111111111110000000000000000000000000000000000000000000000000000ull) >> 52)) - 1024);

		if (exponent > 0) {
			double sum = 1.0;
			while (exponent >= 64) {
				sum *= (1ull << 63);
				exponent -= 64;
			}
			sum *= (1ull << exponent);
			n -= d * sum;
		} else {
			if (n >= d) {
				n -= d;
			} else
				return n;
		}
	}
}
