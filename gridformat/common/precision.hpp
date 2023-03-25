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

inline constexpr Precision<float> float32;
inline constexpr Precision<double> float64;

inline constexpr Precision<std::int_least8_t> int8;
inline constexpr Precision<std::int_least16_t> int16;
inline constexpr Precision<std::int_least32_t> int32;
inline constexpr Precision<std::int_least64_t> int64;

inline constexpr Precision<std::uint_least8_t> uint8;
inline constexpr Precision<std::uint_least16_t> uint16;
inline constexpr Precision<std::uint_least32_t> uint32;
inline constexpr Precision<std::uint_least64_t> uint64;

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
