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
#include <variant>
#include <utility>
#include <type_traits>

#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/concepts.hpp>

namespace GridFormat {

template<typename T>
struct Precision;

#ifndef DOXYGEN
namespace Detail {

template<typename T>
struct PrecisionType;

template<typename T>
struct PrecisionType<Precision<T>> : public std::type_identity<T> {};

}  // namespace Detail
#endif // DOXYGEN

template<Concepts::Scalar T>
struct Precision<T> {
    static constexpr bool is_integral = std::is_integral_v<T>;
    static constexpr bool is_signed = std::is_signed_v<T>;
    static constexpr std::size_t number_of_bytes = sizeof(T);
};

template<>
struct Precision<Automatic> {
    static constexpr bool is_integral = false;
    static constexpr bool is_signed = false;
    static constexpr std::size_t number_of_bytes = 0;
};

using DefaultPrecision = Precision<Automatic>;

template<typename T>
using PrecisionType = Detail::PrecisionType<T>::type;

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

inline constexpr DefaultPrecision default_precision;

class DynamicPrecision {
 public:
    DynamicPrecision() = default;

    template<typename T>
    DynamicPrecision(const Precision<T>& prec)
    : _precision{prec}
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

    std::size_t number_of_bytes() const {
        return std::visit([] <typename T> (const Precision<T>&) {
            return sizeof(T);
        }, _precision);
    }

    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        std::visit(std::forward<Visitor>(visitor), _precision);
    }

 private:
    std::variant<
        Precision<float>,
        Precision<double>,
        Precision<std::int8_t>,
        Precision<std::int16_t>,
        Precision<std::int32_t>,
        Precision<std::int64_t>,
        Precision<std::uint8_t>,
        Precision<std::uint16_t>,
        Precision<std::uint32_t>,
        Precision<std::uint64_t>
    > _precision;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_PRECISION_HPP_