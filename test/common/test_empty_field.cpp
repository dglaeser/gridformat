// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>

#include <gridformat/common/empty_field.hpp>

#include "../testing.hpp"

int main() {

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "empty_field_layout"_test = [&] () {
        GridFormat::EmptyField field{GridFormat::float32};
        expect(eq(field.layout().dimension(), std::size_t{0}));
    };

    "empty_field_precision"_test = [&] () {
        GridFormat::EmptyField field{GridFormat::float32};
        expect(field.precision() == GridFormat::DynamicPrecision{GridFormat::float32});
    };

    "empty_field_serialization"_test = [&] () {
        GridFormat::EmptyField field{GridFormat::float32};
        auto s = field.serialized();
        expect(eq(s.size(), std::size_t{0}));
    };

    return 0;
}
