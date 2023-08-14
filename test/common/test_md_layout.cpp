// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <sstream>

#include <gridformat/common/md_layout.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "md_layout"_test = [] () {
        GridFormat::MDLayout layout{std::vector<int>{1, 2, 3}};
        expect(eq(layout.number_of_entries(), std::size_t{6}));
        expect(eq(layout.extent(0), std::size_t{1}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.extent(2), std::size_t{3}));
        expect(eq(layout.dimension(), std::size_t{3}));
    };

    "md_layout_scalar"_test = [] () {
        const auto layout = GridFormat::get_md_layout(double{});
        expect(eq(layout.number_of_entries(), std::size_t{1}));
        expect(eq(layout.dimension(), std::size_t{1}));
    };

    "md_layout_vector"_test = [] () {
        std::vector<std::array<double, 2>> vector(3);
        const auto layout = GridFormat::get_md_layout(vector);
        expect(eq(layout.dimension(), std::size_t{2}));
        expect(eq(layout.extent(0), std::size_t{3}));
        expect(eq(layout.extent(1), std::size_t{2}));
        expect(eq(layout.number_of_entries(), std::size_t{6}));
    };

    "md_layout_tensor"_test = [] () {
        std::vector<std::array<std::array<double, 2>, 4>> tensor(3);
        const auto layout = GridFormat::get_md_layout(tensor);
        expect(eq(layout.dimension(), std::size_t{3}));
        expect(eq(layout.extent(0), std::size_t{3}));
        expect(eq(layout.extent(1), std::size_t{4}));
        expect(eq(layout.extent(2), std::size_t{2}));
        expect(eq(layout.number_of_entries(), std::size_t{24}));
    };

    "md_layout_output"_test = [] () {
        std::vector<std::array<double, 4>> vector(2);
        const auto layout = GridFormat::get_md_layout(vector);
        std::ostringstream s;
        s << layout;
        expect(eq(s.str(), std::string{"(2,4)"}));
    };

    "md_layout_export"_test = [] () {
        GridFormat::MDLayout layout{{4}};
        std::array<std::size_t, 1> dims{0};
        layout.export_to(dims);
        expect(eq(dims.at(0), std::size_t{4}));
    };

    "md_layout_export_throws_on_too_small_range"_test = [] () {
        GridFormat::MDLayout layout{{4, 1}};
        std::array<std::size_t, 1> dims{0};
        expect(throws<GridFormat::SizeError>([&] () { layout.export_to(dims); }));
    };

    "md_layout_sub_layout_fails_on_too_large_codim"_test = [] () {
        std::vector<std::array<double, 4>> vector(2);
        const auto layout = GridFormat::get_md_layout(vector);
        expect(throws<GridFormat::ValueError>([&] () {
            layout.sub_layout(2);
        }));
    };

    return 0;
}
