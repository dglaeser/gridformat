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
#include <concepts>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/format.hpp>

namespace GridFormat {

template<Concepts::Scalar T>
struct Precision {
    static constexpr bool is_integral = std::is_integral_v<T>;
    static constexpr bool is_signed = std::is_signed_v<T>;
    static constexpr std::size_t number_of_bytes = sizeof(T);
};

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





template<std::integral To, std::integral From>
To cast_to(const Precision<To>&, const From& from) {
    return static_cast<To>(from);
}

template<std::floating_point To, Concepts::Scalar From> requires(
    std::integral<From> or std::floating_point<From>)
To cast_to(const Precision<To>&, const From& from) {
    return static_cast<To>(from);
}


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

// template<typename Action>
// void invoke_with_precision(const PrecisionTraits& prec, Action&& action) {
//     if (prec.is_integral) {
//         if (prec.is_signed) {
//             switch (prec.number_of_bytes) {
//                 case 1: { action(int8); return; }
//                 case 2: { action(int16); return; }
//                 case 4: { action(int32); return; }
//                 case 8: { action(int64); return; }
//             }
//         } else {
//             switch (prec.number_of_bytes) {
//                 case 1: { action(uint8); return; }
//                 case 2: { action(uint16); return; }
//                 case 4: { action(uint32); return; }
//                 case 8: { action(uint64); return; }
//             }
//         }
//     } else {
//         switch (prec.number_of_bytes) {
//             case 4: { action(float32); return; }
//             case 8: { action(float64); return; }
//         }
//     }

//     throw InvalidState("Could not parse given precision traits");
// }

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_PRECISION_HPP_