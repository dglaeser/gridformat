// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gridformat/common/scalar_field.hpp>

#include "../testing.hpp"

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "scalar_field"_test = [] () {
        const GridFormat::ScalarField field{int{42}};
        const auto serialization = field.serialized();
        const auto span = serialization.as_span_of<int>();
        expect(eq(serialization.size(), sizeof(int)));
        expect(eq(span.size(), std::size_t{1}));
        expect(eq(span[0], 42));
    };

    "scalar_field_custom_precision"_test = [] () {
        const GridFormat::ScalarField field{int{42}, GridFormat::Precision<double>{}};
        const auto serialization = field.serialized();
        const auto span = serialization.as_span_of<double>();
        expect(eq(serialization.size(), sizeof(double)));
        expect(eq(span.size(), std::size_t{1}));
        expect(eq(span[0], 42.0));
    };

    return 0;
}
