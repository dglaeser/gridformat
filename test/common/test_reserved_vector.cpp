// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <ranges>
#include <algorithm>

#include <gridformat/common/reserved_vector.hpp>

#include "../testing.hpp"


int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "default_ctor_yields_empty_vector"_test = [] () {
        GridFormat::ReservedVector<double, 3> v;
        expect(eq(v.size(), 0_ul));
    };

    "sized_ctor_yields_filled_vector"_test = [] () {
        GridFormat::ReservedVector<double, 3> v(10, 42.0);
        expect(eq(v.size(), 10_ul));
        expect(std::ranges::equal(v, std::vector<double>(10, 42.0)));
    };

    "initlist_ctor_yields_correct_vector"_test = [] () {
        GridFormat::ReservedVector<double, 3> v{1.0, 2.0, 3.0, 4.0, 5.0};
        expect(eq(v.size(), 5_ul));
        expect(std::ranges::equal(v, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}));

        v = {1.0, 2.0};
        expect(std::ranges::equal(v, std::vector<double>{1.0, 2.0}));
    };

    "cpy_ctor_does_not_copy_resource"_test = [] () {
        GridFormat::ReservedVector<double, 3> v{1.0, 2.0, 3.0};
        GridFormat::ReservedVector<double, 5> cpy{v};
        GridFormat::ReservedVector<double, 10> cpy2 = v;
        v = {0.0, 0.0, 0.0, 0.0};
        expect(eq(cpy.size(), 3_ul));
        expect(eq(cpy2.size(), 3_ul));
        expect(std::ranges::equal(cpy, std::vector<double>{1.0, 2.0, 3.0}));
        expect(std::ranges::equal(cpy2, std::vector<double>{1.0, 2.0, 3.0}));
    };

    "move_ctor_does_not_copy_resource"_test = [] () {
        GridFormat::ReservedVector<double, 3> v = [] () {
            GridFormat::ReservedVector<double, 4> tmp{1.0, 42.0, 43.0};
            GridFormat::ReservedVector<double, 5> moved{std::move(tmp)};
            return moved;
        } ();
        expect(eq(v.size(), 3_ul));
        expect(std::ranges::equal(v, std::vector<double>{1.0, 42.0, 43.0}));
    };

    "push_back_adds_element"_test = [] () {
        GridFormat::ReservedVector<double, 3> v;
        v.push_back(1.0);
        v.push_back(42.0);
        v.push_back(84.0);
        expect(eq(v.size(), 3_ul));
        expect(std::ranges::equal(v, std::vector<double>{1.0, 42.0, 84.0}));
    };

    "accessors_yield_mutable_refs"_test = [] () {
        GridFormat::ReservedVector<double, 3> v(3, 0.0);
        v[0] = 42.0;
        v.at(1) = 84.0;
        v.at(2) = 122.0;
        expect(eq(v.size(), 3_ul));
        expect(std::ranges::equal(v, std::vector<double>{42.0, 84.0, 122.0}));
    };

    "const_iterators"_test = [] () {
        const GridFormat::ReservedVector<double, 3> v{1.0, 42.0, 84.0};
        expect(eq(v.size(), 3_ul));
        expect(std::ranges::equal(v, std::vector<double>{1.0, 42.0, 84.0}));
    };

    "const_accessors"_test = [] () {
        const GridFormat::ReservedVector<double, 3> v{1.0, 42.0, 84.0};
        expect(eq(v.at(0), 1.0));
        expect(eq(v[1], 42.0));
        expect(eq(v.at(2), 84.0));
    };

    return 0;
}
