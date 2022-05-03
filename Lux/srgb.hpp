#include <cmath>
#pragma once
//SRGB conversion routines by TrianglesPCT https://github.com/TrianglesPCT/srgb/blob/master/srgb.h
//designed to be easy to vectorize by only using instructions trivially available with SSE/AVX(avoids pow etc)
//These are the C++ reference implementations
//They are accurate enough to support 8 bit srgb -> 32bit float linear -> 8 bit srgb and produce the original value
//The SIMD variants can also use the 11 bit rcp instructions, as we really do not need full 32 bit float precision here.
//The SIMD code is not provided because it uses my own SSE/AVX library which is too large and complex for someone just wanting some srgb conversion.

//THis code should be auto vectorizable on a not shitty auto vectorizer. 
//MSVC is shitty so don't expect it to auto vectorize this, it is too stupid to handle the conditional despite this being a trivial VBLEND
namespace srgb {
	
	//approximate cubic root
	inline float acbrt2(float x0) {
		int ix = *reinterpret_cast<int*>(&x0);
		ix = ix / 4 + ix / 16;           // Approximate divide by 3.
		ix = ix + ix / 16;
		ix = ix + ix / 256;
		ix = 0x2a5137a0 + ix;        // Initial guess.
		float x = *reinterpret_cast<float*>(&ix);
		x = 0.33333333f*(2.0f*x + x0 / (x*x));  // Newton step.
		return x;
	}
	//This is pow(a, 1/2.4);
	//http://stackoverflow.com/questions/6475373/optimizations-for-pow-with-const-non-integer-exponent
	inline float pow512f_approx_cbrt(float x) {
		float cbrtx = acbrt2(x);
		return cbrtx*sqrt(sqrt(cbrtx));
	}
	
	//Approximate 5th root
	float Fifthroot(float x0) {
		int ix = *reinterpret_cast<int*>(&x0);

		int div5 = (ix >> 2) - (ix >> 4) + (ix >> 6) - (ix >> 8) + (ix >> 10);
		ix = 0x32c82fee + div5;

		//This is an alternative version of the previous 2 lines that uses less instructions(much higher latency instructions though)
		//float fx = float(ix)*0.2f + float(0x32c82fee);
		//ix = int(fx);

		float x = *reinterpret_cast<float*>(&ix);
		float s = x*x;
		x -= (x - x0 / (s*s))*0.2f; //1 newton step does the trick
		return (x);
	}


	inline float linear_to_srgb(float linear) {
		float a = linear * 12.92f;
		float b = 1.055f * pow512f_approx_cbrt(linear) - 0.055f;
		return (linear <= 0.0031308f) ? a : b;
	}

	//This can also be done with a LUT as there are only 256 possible inputs
	float srgb_to_linear(int v) {
		float srgb = float(v) / 255.f;
		float c0(1.0f / 12.92f);
		float c1(1.f / 1.055f);
		float a = srgb * c0;
		float p1 = (srgb*c1 + 0.055f / 1.055f);
		float b = (p1*p1) * Fifthroot(p1*p1);
		return srgb < 0.04045f ? a : b;
	}
}