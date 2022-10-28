// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup Common
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_COMMON_FORMAT_HPP_
#define GRIDFORMAT_COMMON_FORMAT_HPP_

#include <string_view>
#include <concepts>
#include <iostream>
#include <string>
#include <vector>

namespace GridFormat::Format {

#ifndef DOXYGEN
namespace Detail {

struct AnsiiCodes {
    std::vector<int> codes;

    std::string format(std::string_view msg) const {
        std::string result;
        for (auto _code : codes)
            result += _format_code(_code);
        result += msg;
        result += _format_code(0);
        return result;
    }

 private:
    std::string _format_code(int code) const {
        return std::string{"\033["} + std::to_string(code) + "m";
    }
};

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup Common
 * \brief TODO: Doc me
 */
std::string as_warning(std::string_view msg) {
    Detail::AnsiiCodes codes{{1, 33}};
    return codes.format(msg);
}

}  // namespace GridFormat::Colors

#endif  // GRIDFORMAT_COMMON_FORMAT_HPP_