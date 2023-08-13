// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#ifndef GRIDFORMAT_BIN_COMMON_HPP_
#define GRIDFORMAT_BIN_COMMON_HPP_

#include <ranges>
#include <algorithm>
#include <optional>

namespace GridFormat::Apps {

bool args_ask_for_help(int argc, char** argv) {
    return std::ranges::any_of(
        std::views::iota(1, argc)
        | std::views::transform([&] (int i) { return std::string{argv[i]}; }),
        [] (const std::string& arg) { return arg == "--help"; }
    );
}

std::string wrapped(std::string word, std::string prefix, std::optional<std::string> suffix = {}) {
    auto suffix_str = std::move(suffix).value_or(prefix);
    prefix.reserve(prefix.size() + word.size() + suffix_str.size());
    std::ranges::move(std::move(word), std::back_inserter(prefix));
    std::ranges::move(std::move(suffix_str), std::back_inserter(prefix));
    return prefix;
}

std::string as_cell(std::string word, std::size_t count = 20) {
    if (word.size() < count)
        word.resize(count, ' ');
    return word;
}

}  // namespace GridFormat::Apps

#endif  // GRIDFORMAT_BIN_COMMON_HPP_
