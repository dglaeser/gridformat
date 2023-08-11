// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for casting types from and to strings.
 */
#ifndef GRIDFORMAT_COMMON_STRING_CONVERSION_HPP_
#define GRIDFORMAT_COMMON_STRING_CONVERSION_HPP_

#include <concepts>
#include <string>
#include <sstream>
#include <utility>
#include <locale>
#include <ranges>
#include <string_view>
#include <algorithm>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>

namespace GridFormat {

//! \addtogroup Common
//! @{

/*!
 * \name String conversions
 *       Helper functions for obtaining a string representation of objects.
 *       Users may provide an overload for their types to be consumed by GridFormat.
 */
//!@{

template<Concepts::Scalar T>
inline std::string as_string(const T& t) {
    return std::to_string(t);
}

template<typename T> requires std::constructible_from<std::string, T>
inline std::string as_string(T&& t) {
    return std::string{std::forward<T>(t)};
}

inline const std::string& as_string(const std::string& t) {
    return t;
}

template<std::ranges::range R> requires(!std::constructible_from<std::string, R>)
inline std::string as_string(R&& range, std::string_view delimiter = " ") {
    std::string result;
    std::ranges::for_each(range, [&] (const auto& entry) {
        result += delimiter;
        result += as_string(entry);
    });
    result.erase(0, delimiter.size());
    return result;
}

template<Concepts::Scalar T>
T from_string(const std::string& str) {
    if constexpr (std::is_same_v<T, char>)
        return static_cast<T>(std::stoi(str));

    std::istringstream s(str);
    s.imbue(std::locale::classic());

    T val;
    s >> val;
    if (!s)
        throw ValueError("Could not extract value of requested type from '" + str + "'");

    // make sure subsequent extraction fails
    char dummy;
    s >> dummy;
    if (!s.fail() || !s.eof())
        throw ValueError("Value extraction of requested type from string '" + str + "' unsuccessful");

    return val;
}

template<std::convertible_to<std::string> T>
inline T from_string(const std::string& t) {
    return T{t};
}

//! @} converter functions
//! @} Common group

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_STRING_CONVERSION_HPP_
