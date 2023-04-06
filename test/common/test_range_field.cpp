// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <ranges>

#include <gridformat/common/range_field.hpp>

#include "../testing.hpp"

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "range_field"_test = [] () {
        const GridFormat::RangeField field{std::vector<int>{1, 2, 3, 4}};
        const auto serialization = field.serialized();
        const auto span = serialization.as_span_of<int>();
        expect(eq(serialization.size(), 4*sizeof(int)));
        expect(eq(span.size(), std::size_t{4}));
        expect(std::ranges::equal(span, std::vector<int>{1, 2, 3, 4}));
    };

    "range_field_custom_precision"_test = [] () {
        const GridFormat::RangeField field{
            std::vector<int>{1, 2, 3, 4},
            GridFormat::Precision<double>{}
        };
        const auto serialization = field.serialized();
        const auto span = serialization.as_span_of<double>();
        expect(eq(serialization.size(), 4*sizeof(double)));
        expect(eq(span.size(), std::size_t{4}));
        expect(std::ranges::equal(span, std::vector<double>{1., 2., 3., 4.}));
    };

    "range_field_by_reference"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        const GridFormat::RangeField field{data};
        data[0] = 0;
        const auto serialization = field.serialized();
        const auto ints = serialization.as_span_of<int>();
        expect(std::ranges::equal(data, ints));
    };

    "range_field_by_reference_custom_precision"_test = [] () {
        std::vector<int> data{1, 2, 3, 4};
        const GridFormat::RangeField field{data, GridFormat::Precision<double>{}};
        data[0] = 0;
        const auto serialization = field.serialized();
        const auto doubles = serialization.as_span_of<double>();
        expect(std::ranges::equal(data, doubles));
    };

    return 0;
}
