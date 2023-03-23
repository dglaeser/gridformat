// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief Functionality for styling and logging strings.
 */
#ifndef GRIDFORMAT_COMMON_LOGGING_HPP_
#define GRIDFORMAT_COMMON_LOGGING_HPP_

#include <array>
#include <utility>
#include <concepts>
#include <iostream>
#include <string_view>
#include <string>

namespace GridFormat {

//! \addtogroup Common
//! \{

#ifndef DOXYGEN
namespace Detail {

    template<std::size_t size>
    struct AnsiiCodes {
        template<std::integral... Args> requires(sizeof...(Args) == size)
        constexpr explicit AnsiiCodes(Args&&... args) {
            if constexpr (size > 0)
                _set_code<0>(std::forward<Args>(args)...);
        }

        std::string format(std::string_view msg) const {
            std::string result;
            for (auto _code : _codes)
                result += _format_code(_code);
            result += msg;
            result += _format_code(0);
            return result;
        }

    private:
        std::array<int, size> _codes;

        template<int i, std::integral T, std::integral... Ts>
        constexpr void _set_code(T&& code, Ts&&... codes) {
            _codes[i] = code;
            if constexpr (sizeof...(Ts) > 0)
                _set_code<i+1>(std::forward<Ts>(codes)...);
        }

        std::string _format_code(int code) const {
            return std::string{"\033["} + std::to_string(code) + "m";
        }
    };

    template<std::integral... Args>
    AnsiiCodes(Args&&... args) -> AnsiiCodes<sizeof...(Args)>;

}  // namespace Detail
#endif  // DOXYGEN

//! Style the given string as a warning
std::string as_warning(std::string_view msg) {
    static constexpr Detail::AnsiiCodes codes{1, 33};
    return codes.format(msg);
}

//! Style the given string as an error
std::string as_error(std::string_view msg) {
    static constexpr Detail::AnsiiCodes codes{1, 31};
    return codes.format(msg);
}

//! Style the given string as highlighted
std::string as_highlight(std::string_view msg) {
    static constexpr Detail::AnsiiCodes codes{1};
    return codes.format(msg);
}

//! Log a warning message.
void log_warning(std::string_view msg, std::ostream& s = std::cout) {
    s << "[GFMT] " << as_warning("Warning") << ": " << msg << "\n";
}

//! \} group Common

}  // namespace GridFormat

#endif  // GRIDFORMAT_COMMON_LOGGING_HPP_
