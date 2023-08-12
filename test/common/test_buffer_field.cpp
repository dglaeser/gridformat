// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>

#include <gridformat/common/buffer_field.hpp>

#include "../testing.hpp"

int main() {

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    std::vector<int> values{1, 2, 3, 4, 5, 6};

    "buffer_field_1d"_test = [&] () {
        GridFormat::BufferField field{values, GridFormat::MDLayout{{6}}};
        auto s = field.serialized();
        expect(std::ranges::equal(
            s.as_span_of(GridFormat::Precision<int>{}),
            values
        ));
    };

    "buffer_field_2d"_test = [&] () {
        GridFormat::BufferField field{values, GridFormat::MDLayout{{2, 3}}};
        auto s = field.serialized();
        expect(std::ranges::equal(
            s.as_span_of(GridFormat::Precision<int>{}),
            values
        ));
    };

    "buffer_field_3d"_test = [&] () {
        GridFormat::BufferField field{values, GridFormat::MDLayout{{1, 2, 3}}};
        auto s = field.serialized();
        expect(std::ranges::equal(
            s.as_span_of(GridFormat::Precision<int>{}),
            values
        ));
    };

    "buffer_field_4d"_test = [&] () {
        GridFormat::BufferField field{values, GridFormat::MDLayout{{1, 2, 3, 1}}};
        auto s = field.serialized();
        expect(std::ranges::equal(
            s.as_span_of(GridFormat::Precision<int>{}),
            values
        ));
    };

    "buffer_field_wrong_layout"_test = [&] () {
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::BufferField{values, GridFormat::MDLayout{{1, 2, 3, 2}}};
        }));
    };

    return 0;
}
