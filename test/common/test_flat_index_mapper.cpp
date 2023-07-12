// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <array>

#include <gridformat/common/flat_index_mapper.hpp>

#include "../testing.hpp"

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "flat_index_mapper_1d_default"_test = [] () {
        GridFormat::FlatIndexMapper<1> mapper{};
        expect(eq(mapper.map(std::array{0}), 0_ul));
        expect(eq(mapper.map(std::array{1}), 1_ul));
        expect(eq(mapper.map(std::array{2}), 2_ul));
        expect(eq(mapper.map(std::array{3}), 3_ul));
    };

    "flat_index_mapper_1d_with_arg"_test = [] () {
        GridFormat::FlatIndexMapper mapper{std::array{4}};
        expect(eq(mapper.map(std::array{0}), 0));
        expect(eq(mapper.map(std::array{1}), 1));
        expect(eq(mapper.map(std::array{2}), 2));
        expect(eq(mapper.map(std::array{3}), 3));
    };

    "flat_index_mapper_2d"_test = [] () {
        GridFormat::FlatIndexMapper mapper{std::array{2, 3}};
        expect(eq(mapper.map(std::array{0, 0}), 0));
        expect(eq(mapper.map(std::array{1, 0}), 1));
        expect(eq(mapper.map(std::array{0, 1}), 2));
        expect(eq(mapper.map(std::array{1, 1}), 3));
        expect(eq(mapper.map(std::array{0, 2}), 4));
        expect(eq(mapper.map(std::array{1, 2}), 5));
    };

    "flat_index_mapper_2d_dynamic"_test = [] () {
        GridFormat::FlatIndexMapper mapper{std::vector{2, 3}};
        expect(eq(mapper.map(std::array{0, 0}), 0));
        expect(eq(mapper.map(std::vector{1, 0}), 1));
        expect(eq(mapper.map(std::vector{0, 1}), 2));
        expect(eq(mapper.map(std::array{1, 1}), 3));
        expect(eq(mapper.map(std::array{0, 2}), 4));
        expect(eq(mapper.map(std::vector{1, 2}), 5));
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::FlatIndexMapper<2> mapper{std::vector{2}};
        }));
    };

    "flat_index_mapper_3d"_test = [] () {
        GridFormat::FlatIndexMapper mapper{std::array{2, 3, 2}};
        expect(eq(mapper.map(std::array{0, 0, 0}), 0));
        expect(eq(mapper.map(std::array{1, 0, 0}), 1));
        expect(eq(mapper.map(std::array{0, 1, 0}), 2));
        expect(eq(mapper.map(std::array{1, 1, 0}), 3));
        expect(eq(mapper.map(std::array{0, 2, 0}), 4));
        expect(eq(mapper.map(std::array{1, 2, 0}), 5));
        expect(eq(mapper.map(std::array{0, 0, 1}), 6));
        expect(eq(mapper.map(std::array{1, 0, 1}), 7));
        expect(eq(mapper.map(std::array{0, 1, 1}), 8));
        expect(eq(mapper.map(std::array{1, 1, 1}), 9));
        expect(eq(mapper.map(std::array{0, 2, 1}), 10));
        expect(eq(mapper.map(std::array{1, 2, 1}), 11));
    };

    return 0;
}
