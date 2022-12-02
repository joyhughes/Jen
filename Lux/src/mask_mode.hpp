#ifndef __MASK_MODE_HPP
#define __MASK_MODE_HPP

// Formula in comments - background B, mask M, splat S
// least significant bit - mask affects image. 2's bit - mask affects background
enum mask_mode
{
	MASK_ADDITIVE,  // B + S - mask has no effect
	MASK_TRIM,      // B + MS - mask only affects splat
	MASK_OPACITY,   // (1-M)B + S - mask only affects background
	MASK_BLEND,     // (1-M)B + MS - linear interpolation (extrapolation allowed)
};
typedef enum mask_mode mask_mode;

#endif // __MASK_MODE_HPP