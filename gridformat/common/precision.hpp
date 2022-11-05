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

class DynamicPrecision {
 public:
    template<typename T>
    DynamicPrecision(const Precision<T>&)
    : _is_integral{std::is_integral_v<T>}
    , _is_signed{std::is_signed_v<T>}
    , _number_of_bytes{sizeof(T)}
    {}

    bool operator==(const DynamicPrecision& other) const {
        return _is_integral == other._is_integral
            && _is_signed == other._is_signed
            && _number_of_bytes == other._number_of_bytes;
    }

 private:
    bool _is_integral;
    bool _is_signed;
    std::size_t _number_of_bytes;
};

template<typename T>
DynamicPrecision as_dynamic(const Precision<T>& prec) {
    return DynamicPrecision{prec};
}

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_SCALAR_HPP_