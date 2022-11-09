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
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/format.hpp>

namespace GridFormat {

template<typename T>
struct Precision {};

template<std::integral To, std::integral From>
To cast_to(const Precision<To>&, const From& from) {
    return static_cast<To>(from);
}

template<std::floating_point To, Concepts::Scalar From> requires(
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

struct PrecisionTraits {
    bool is_integral;
    bool is_signed;
    std::size_t number_of_bytes;

    template<typename T>
    PrecisionTraits(const Precision<T>&)
    : is_integral{std::is_integral_v<T>}
    , is_signed{std::is_signed_v<T>}
    , number_of_bytes{sizeof(T)}
    {}

    bool operator==(const PrecisionTraits& other) const {
        return is_integral == other.is_integral
            && is_signed == other.is_signed
            && number_of_bytes == other.number_of_bytes;
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_PRECISION_HPP_