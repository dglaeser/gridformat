// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>

#include <gridformat/common/reserved_string.hpp>

#include "../testing.hpp"


int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "reserved_string_from_literal"_test = [] () {
        GridFormat::ReservedString str{"hello"};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
        static_assert(std::ranges::range<decltype(str)>);
        static_assert(std::is_same_v<decltype(str), GridFormat::ReservedString<5>>);
    };

    "reserved_string_from_literal_with_max_size"_test = [] () {
        GridFormat::ReservedString<5> str{"hello"};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_from_string"_test = [] () {
        GridFormat::ReservedString str{std::string{"hello"}};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_from_string_view"_test = [] () {
        GridFormat::ReservedString str{std::string_view{"hello"}};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_from_string_with_exceeding_size"_test = [] () {
        expect(throws<GridFormat::SizeError>([] () {
            GridFormat::ReservedString<4> str{std::string{"hello"}};
        }));
    };

    "reserved_string_from_string_view_with_exceeding_size"_test = [] () {
        expect(throws<GridFormat::SizeError>([] () {
            GridFormat::ReservedString<4> str{std::string_view{"hello"}};
        }));
    };

    "reserved_string_assign_from_literal"_test = [] () {
        GridFormat::ReservedString str;
        str = "hello";
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_assign_from_string_view"_test = [] () {
        GridFormat::ReservedString str;
        str = std::string_view{"hello"};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_assign_from_string"_test = [] () {
        GridFormat::ReservedString str;
        str = std::string{"hello"};
        expect(eq(str.size(), 5_ul));
        expect(eq(std::string_view{str}, std::string_view{"hello"}));
    };

    "reserved_string_ctor_throws_on_missing_null_terminator"_test = [] () {
        char data[4] = {'a', 'b', 'c', 'd'};
        expect(throws<GridFormat::ValueError>([&] () { GridFormat::ReservedString{data}; }));
    };

    "reserved_string_eq_operator"_test = [] () {
        GridFormat::ReservedString<10> a{"hello"};
        GridFormat::ReservedString<10> b{"hello"};
        GridFormat::ReservedString<10> c{"hell"};
        expect(a == b);
        expect(a != c);
        expect(c != a);
    };

    return 0;
}
