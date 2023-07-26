// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <gridformat/common/optional_reference.hpp>
#include "../testing.hpp"

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;

    "optional_reference_none"_test = [] () {
        GridFormat::OptionalReference<double> opt_ref;
        expect(!opt_ref.has_value());
        expect(!opt_ref);
    };

    "optional_reference_with_value"_test = [] () {
        double value = 1.0;
        GridFormat::OptionalReference opt_ref{value};
        expect(eq(opt_ref.unwrap(), value));
        value = 42.0;
        expect(eq(opt_ref.unwrap(), value));
    };

    "optional_const_reference_with_value"_test = [] () {
        const double value = 33.0;
        GridFormat::OptionalReference opt_ref{value};
        expect(eq(opt_ref.unwrap(), value));
    };

    "optional_reference_release"_test = [] () {
        const double value = 33.0;
        GridFormat::OptionalReference opt_ref{value};
        expect(eq(opt_ref.unwrap(), value));
        opt_ref.release();
        expect(!opt_ref.has_value());
    };

    return 0;
}
