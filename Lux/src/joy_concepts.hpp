#ifndef __JOY_CONCEPTS_HPP__
#define __JOY_CONCEPTS_HPP__

#include <concepts>
#include <type_traits>

template<typename T, typename... Ts>
concept IsOneOf = (std::is_same_v<T, Ts> || ...);

template<typename T> 
concept Scalar = std::is_arithmetic_v<T>;

template<typename T>
concept Additive = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
};

template<typename T>
concept MultipliableByFloat = requires(T a, float b) {
    { a * b } -> std::same_as<T>;
};

#endif // __JOY_CONCEPTS_HPP__
