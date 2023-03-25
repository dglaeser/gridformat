// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Types to represent different precisions.
 */
#ifndef GRIDFORMAT_COMMON_PRECISION_HPP_
#define GRIDFORMAT_COMMON_PRECISION_HPP_

#include <cstdint>
#include <variant>
#include <utility>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

/*!
 * \ingroup Common
 * \brief Represents a precision known at compile-time
 */
template<Concepts::Scalar T>
struct Precision {};

using Float32 = Precision<float>;
using Float64 = Precision<double>;
inline constexpr Float32 float32;
inline constexpr Float64 float64;

using Int8 = Precision<std::int_least8_t>;
using Int16 = Precision<std::int_least16_t>;
using Int32 = Precision<std::int_least32_t>;
using Int64 = Precision<std::int_least64_t>;
inline constexpr Int8 int8;
inline constexpr Int16 int16;
inline constexpr Int32 int32;
inline constexpr Int64 int64;

using UInt8 = Precision<std::uint_least8_t>;
using UInt16 = Precision<std::uint_least16_t>;
using UInt32 = Precision<std::uint_least32_t>;
using UInt64 = Precision<std::uint_least64_t>;
inline constexpr UInt8 uint8;
inline constexpr UInt16 uint16;
inline constexpr UInt32 uint32;
inline constexpr UInt64 uint64;

inline constexpr Precision<std::size_t> default_integral;
inline constexpr Precision<double> default_floating_point;

/*!
 * \ingroup Common
 * \brief Represents a dynamic precision.
 * \note This can only represent the precisions predefined in this header.
 */
class DynamicPrecision {
 public:
    DynamicPrecision() = default;

    template<typename T>
    DynamicPrecision(Precision<T> prec)
    : _precision{std::move(prec)}
    {}

    bool is_integral() const {
        return std::visit([] <typename T> (const Precision<T>&) {
            return std::is_integral_v<T>;
        }, _precision);
    }

    bool is_signed() const {
        return std::visit([] <typename T> (const Precision<T>&) {
            return std::is_signed_v<T>;
        }, _precision);
    }

    std::size_t size_in_bytes() const {
        return std::visit([] <typename T> (const Precision<T>&) {
            return sizeof(T);
        }, _precision);
    }

    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), _precision);
    }

 private:
    UniqueVariant<
        Precision<float>,
        Precision<double>,
        Precision<std::int8_t>,
        Precision<std::int16_t>,
        Precision<std::int32_t>,
        Precision<std::int64_t>,
        Precision<std::uint8_t>,
        Precision<std::uint16_t>,
        Precision<std::uint32_t>,
        Precision<std::uint64_t>,
        Precision<std::size_t>,
        Precision<char>
    > _precision;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_PRECISION_HPP_
