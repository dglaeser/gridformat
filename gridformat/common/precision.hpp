// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_PRECISION_HPP_
#define GRIDFORMAT_COMMON_PRECISION_HPP_

#include <cstdint>
#include <concepts>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/format.hpp>

namespace GridFormat {

template<typename T>
struct Precision {};

template<std::integral To, std::integral From>
To cast_to(const Precision<To>&, const From& from) {
    return static_cast<To>(from);
}

template<std::floating_point To, typename From> requires(
    std::integral<From> or std::floating_point<From>)
To cast_to(const Precision<To>&, const From& from) {
    return static_cast<To>(from);
}

inline constexpr Precision<float> float32;
inline constexpr Precision<double> float64;

inline constexpr Precision<std::int8_t> int8;
inline constexpr Precision<std::int16_t> int16;
inline constexpr Precision<std::int32_t> int32;
inline constexpr Precision<std::int64_t> int64;

inline constexpr Precision<std::uint8_t> uint8;
inline constexpr Precision<std::uint16_t> uint16;
inline constexpr Precision<std::uint32_t> uint32;
inline constexpr Precision<std::uint64_t> uint64;

enum class DynamicPrecision {
    float32, float64,
    int8, int16, int32, int64,
    uint8, uint16, uint32, uint64
};

template<typename T>
DynamicPrecision as_dynamic(const Precision<T>&) {
    if constexpr (std::same_as<T, float>) return DynamicPrecision::float32;
    if constexpr (std::same_as<T, double>) return DynamicPrecision::float64;

    if constexpr (std::same_as<T, std::int8_t>) return DynamicPrecision::int8;
    if constexpr (std::same_as<T, std::int16_t>) return DynamicPrecision::int16;
    if constexpr (std::same_as<T, std::int32_t>) return DynamicPrecision::int32;
    if constexpr (std::same_as<T, std::int64_t>) return DynamicPrecision::int64;

    if constexpr (std::same_as<T, std::uint8_t>) return DynamicPrecision::uint8;
    if constexpr (std::same_as<T, std::uint16_t>) return DynamicPrecision::uint16;
    if constexpr (std::same_as<T, std::uint32_t>) return DynamicPrecision::uint32;
    if constexpr (std::same_as<T, std::uint64_t>) return DynamicPrecision::uint64;

    throw InvalidState("Unknown precision format");
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SCALAR_HPP_