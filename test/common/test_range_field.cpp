// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <ranges>
#include <tuple>

#include <gridformat/common/range_field.hpp>
#include <gridformat/common/exceptions.hpp>

#include "../testing.hpp"

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
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

    "range_field_export"_test = [] () {
        const GridFormat::RangeField field{
            std::vector<std::array<int, 3>>{{0, 1, 2}, {3, 4, 5}}
        };
        std::vector<std::array<double, 3>> out(2);
        field.export_to(out);
        expect(std::ranges::equal(out[0], std::vector<double>{0.0, 1.0, 2.0}));
        expect(std::ranges::equal(out[1], std::vector<double>{3.0, 4.0, 5.0}));
    };

    "range_field_flat_export"_test = [] () {
        const GridFormat::RangeField field{
            std::vector<std::array<int, 3>>{{0, 1, 2}, {3, 4, 5}}
        };
        std::vector<double> out(6);
        field.export_to(out);
        expect(std::ranges::equal(out, std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}));
    };

    "range_field_reshaped_export"_test = [] () {
        const GridFormat::RangeField field{
            std::vector<int>{{0, 1, 2, 3, 4, 5}}
        };
        std::vector<std::array<double, 3>> out(2);
        field.export_to(out);
        expect(std::ranges::equal(out[0], std::vector<double>{0.0, 1.0, 2.0}));
        expect(std::ranges::equal(out[1], std::vector<double>{3.0, 4.0, 5.0}));
    };

    "range_field_direct_export"_test = [] () {
        const GridFormat::RangeField field{
            std::vector<int>{{0, 1, 2, 3, 4, 5}}
        };

        using DoubleVec = std::vector<double>;
        using ArrayVec = std::vector<std::array<double, 3>>;
        std::vector<std::tuple<DoubleVec, ArrayVec>> exports{
            std::make_tuple(field.template export_to<DoubleVec>(), field.template export_to<ArrayVec>()),
            std::make_tuple(field.export_to(DoubleVec{}), field.export_to(ArrayVec{})),
        };
        for (const auto& [double_vec, array_vec] : exports) {
            expect(std::ranges::equal(double_vec, std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}));

            expect(std::ranges::equal(array_vec[0], std::vector<double>{0.0, 1.0, 2.0}));
            expect(std::ranges::equal(array_vec[1], std::vector<double>{3.0, 4.0, 5.0}));
        }

        expect(throws<GridFormat::TypeError>([&] () {
            const auto fail = field.template export_to<std::vector<std::array<double, 4>>>();
        }));
        expect(throws<GridFormat::TypeError>([&] () {
            const auto fail = field.export_to(std::vector<std::array<double, 4>>{});
        }));
    };

    return 0;
}
