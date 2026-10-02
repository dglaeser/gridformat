// SPDX-FileCopyrightText: 2026 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include <gridformat/common/multi_string.hpp>
#include <gridformat/common/range_field.hpp>

#include "../testing.hpp"

using Strings = std::vector<std::string_view>;

GridFormat::MultiString from_chars(std::string_view chars) {
    GridFormat::MultiString result;
    result.resize(chars.size());
    std::ranges::copy(chars, result.begin());
    return result;
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "multi_string_from_strings"_test = [] () {
        const GridFormat::MultiString strings{{"first", "", "third"}};
        expect(eq(std::string{strings.begin(), strings.end()}, std::string{"first\0\0third\0", 13}));
        expect(strings.slices() == Strings{"first", "", "third"});
    };

    "multi_string_slices"_test = [] () {
        using namespace std::string_view_literals;
        const auto slices_of = [] (std::string_view chars) {
            const auto strings = from_chars(chars);
            const auto slices = strings.slices();
            return std::vector<std::string>{slices.begin(), slices.end()};
        };
        expect(slices_of("a\0b\0"sv) == std::vector<std::string>{"a", "b"});
        expect(slices_of("a\0b"sv) == std::vector<std::string>{"a", "b"});
        expect(slices_of("abc"sv) == std::vector<std::string>{"abc"});
        expect(slices_of("\0"sv) == std::vector<std::string>{""});
        expect(slices_of("\0\0"sv) == std::vector<std::string>{"", ""});
    };

    "multi_string_empty_is_a_single_empty_string"_test = [] () {
        const GridFormat::MultiString empty;
        const GridFormat::MultiString from_empty_vector{std::vector<std::string>{}};
        expect(empty.slices() == Strings{""});
        expect(from_empty_vector.slices() == Strings{""});
    };

    "multi_string_field_round_trip"_test = [] () {
        const GridFormat::MultiString strings{{"first", "", "third"}};
        const GridFormat::RangeField field{strings};
        expect(field.precision().template is<char>());
        expect(eq(field.layout().number_of_entries(), strings.size()));
        expect(field.export_to<GridFormat::MultiString>() == strings);
    };

    "field_export_to_string"_test = [] () {
        using namespace std::string_view_literals;
        const auto export_chars = [] (std::string_view chars) {
            return GridFormat::RangeField{from_chars(chars)}.template export_to<std::string>();
        };
        expect(eq(export_chars("text"sv), std::string{"text"}));
        expect(eq(export_chars("text\0"sv), std::string{"text"}));
        expect(eq(export_chars(""sv), std::string{}));
        expect(throws([&] () { export_chars("a\0b\0"sv); }));
        expect(throws([&] () { export_chars("a\0b"sv); }));
        expect(throws([&] () { export_chars("text\0\0"sv); }));
    };

    return 0;
}
