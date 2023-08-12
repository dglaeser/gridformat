// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <variant>
#include <type_traits>
#include <filesystem>
#include <ranges>

#include <gridformat/common/path.hpp>
#include <gridformat/common/ranges.hpp>

#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "test_is_file_fails_on_directory"_test = [] () {
        expect(std::filesystem::is_directory("."));
        expect(!GridFormat::Path::is_file("."));
    };

    "test_is_file_succeeds_on_symlink"_test = [] () {
        bool visited = false;
        for (const std::filesystem::path& file
                : std::filesystem::directory_iterator{"."}
                | std::views::filter([] (const auto& p) { return std::filesystem::is_regular_file(p); })) {
            visited = true;
            std::filesystem::path symlink_path = std::filesystem::path{file.filename().string() + "_test_path_symlink_test"};
            std::filesystem::create_symlink(file, symlink_path);
            expect(GridFormat::Path::is_file(symlink_path));
            std::filesystem::remove(symlink_path);
            break;
        }
        expect(visited);
    };

    return 0;
}
