#include <random>

static std::random_device rd;   // non-deterministic generator
static std::mt19937 gen( rd() ); // start random engine
static std::uniform_real_distribution<float> rand1( 0.0f, 1.0f );
static float rand_range( float a, float b ) { return a + ( b - a ) * rand1(gen); }