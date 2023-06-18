// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <variant>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/variant.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "variant_without_single_type"_test = [] () {
        std::variant<int, char> v{int{42}};
        const auto only_int = GridFormat::Variant::without<char>(v);
        static_assert(std::is_same_v<std::decay_t<decltype(only_int)>, std::variant<int>>);
        std::visit([] (const int value) { return expect(eq(value, 42)); }, only_int);
    };

    "variant_without_multiple_types"_test = [] () {
        std::variant<int, double, char> v{int{42}};
        const auto only_int = GridFormat::Variant::without<char, double>(v);
        static_assert(std::is_same_v<std::decay_t<decltype(only_int)>, std::variant<int>>);
        std::visit([] (const int value) { return expect(eq(value, 42)); }, only_int);
    };

    return 0;
}
