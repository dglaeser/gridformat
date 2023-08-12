// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#ifndef GRIDFORMAT_BIN_COMMON_HPP_
#define GRIDFORMAT_BIN_COMMON_HPP_

#include <ranges>
#include <algorithm>

namespace GridFormat::Apps {

bool args_ask_for_help(int argc, char** argv) {
    return std::ranges::any_of(
        std::views::iota(1, argc)
        | std::views::transform([&] (int i) { return std::string{argv[i]}; }),
        [] (const std::string& arg) { return arg == "--help"; }
    );
}

}  // namespace GridFormat::Apps

#endif  // GRIDFORMAT_BIN_COMMON_HPP_
