#ifndef COLORS_HPP
#define COLORS_HPP

#include "ucolor.hpp"
#include "frgb.hpp"
#include "vect2.hpp"

// Primary template
template <typename T> constexpr T black = T{};
template <typename T> constexpr T white = T{};

// Specialization for specific types
template <> inline constexpr int black<unsigned char> = 0;
template <> inline constexpr int white<unsigned char> = 255;

template <> inline constexpr int black<int> = 0;
template <> inline constexpr int white<int> = 255;

template <> inline constexpr float black<float> = 0.0f;
template <> inline constexpr float white<float> = 1.0f;

template <> inline constexpr vec2i black<vec2i> = { 0, 0 };
template <> inline constexpr vec2i white<vec2i> = { 255, 255 };

template <> inline constexpr vec2f black<vec2f> = { 0.0f, 0.0f };
template <> inline constexpr vec2f white<vec2f> = { 1.0f, 1.0f };

template <> inline constexpr frgb black<frgb> = { 0.0f, 0.0f, 0.0f };
template <> inline constexpr frgb white<frgb> = { 1.0f, 1.0f, 1.0f };

template <> inline constexpr ucolor black<ucolor> = { 0xff000000 };
template <> inline constexpr ucolor white<ucolor> = { 0xffffffff };

#endif // COLORS_HPP