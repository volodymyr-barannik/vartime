#pragma once
#include <cstdint>
#include <type_traits>

template <intmax_t val>
using intmax_constant = std::integral_constant <intmax_t, val>;

template<intmax_t decimalNumber>
struct bitcount : intmax_constant<1 + bitcount<(decimalNumber >> 1)>::value> {};

template<>
struct bitcount<0> : intmax_constant<1> {};

template<>
struct bitcount<1> : intmax_constant<1> {};

constexpr intmax_t bits_to_bytes(intmax_t bits) noexcept
{
	return (bits + 7) / 8; 
}

template<intmax_t decimalNumber>
struct bytecount : intmax_constant < (bits_to_bytes(bitcount<decimalNumber>::value))> {};


template<intmax_t bytes>
struct min_suitable_uint_type : std::type_identity<intmax_t> {};

template<>
struct min_suitable_uint_type<4> : std::type_identity<uint32_t> {};

template<>
struct min_suitable_uint_type<3> : std::type_identity<uint32_t> {};

template<>
struct min_suitable_uint_type<2> : std::type_identity<uint16_t> {};

template<>
struct min_suitable_uint_type<1> : std::type_identity<uint8_t> {};
