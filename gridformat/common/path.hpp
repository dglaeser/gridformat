// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helper functions for operations on paths.
 */
#ifndef GRIDFORMAT_COMMON_PATH_HPP_
#define GRIDFORMAT_COMMON_PATH_HPP_

#include <ranges>
#include <iterator>
#include <string_view>
#include <filesystem>
#include <algorithm>

namespace GridFormat::Path {

/*!
 * \ingroup Common
 * \brief Return a range over the elements of a path.
 */
std::ranges::range auto elements_of(std::string_view path, char delimiter = '/') {
    return std::views::split(path, delimiter)
        | std::views::transform([] (const std::ranges::range auto& element) {
            std::string name;
            std::ranges::copy(element, std::back_inserter(name));
            return name;
        });
}

//! Return true if the given path exists.
bool exists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

//! Return true if the given path is a file.
bool is_file(const std::filesystem::path& path) {
    if (std::filesystem::is_regular_file(path))
        return true;
    if (std::filesystem::is_symlink(path)
        && std::filesystem::is_regular_file(std::filesystem::read_symlink(path)))
        return true;
    return false;
}

}  // end namespace GridFormat::Path

#endif  // GRIDFORMAT_COMMON_PATH_HPP_
