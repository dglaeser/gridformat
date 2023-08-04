// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <iostream>

#include <gridformat/common/precision.hpp>
#include "../testing.hpp"

template<typename T>
void _test(const GridFormat::Precision<T>& prec) {
    GridFormat::DynamicPrecision precision;
    precision = prec;

    std::cout << "Testing precision " << precision << std::endl;

    using GridFormat::Testing::expect;
    using GridFormat::Testing::eq;
    expect(eq(precision.is_integral(), std::is_integral_v<T>));
    expect(eq(precision.is_signed(), std::is_signed_v<T>));
    expect(eq(precision.size_in_bytes(), sizeof(T)));
    precision.visit([&] <typename _T> (const GridFormat::Precision<_T>& _p) {
        expect(std::is_same_v<T, _T>);
        expect(precision == _p);
        if constexpr (!std::is_same_v<_T, double>)
            expect(precision != GridFormat::DynamicPrecision{GridFormat::Precision<double>{}});
    });
}

int main() {
    using GridFormat::Testing::operator""_test;

    "dynamic_precision_float32"_test = [&] () { _test(GridFormat::float32); };
    "dynamic_precision_float64"_test = [&] () { _test(GridFormat::float64); };
    "dynamic_precision_int8"_test = [&] () { _test(GridFormat::int8); };
    "dynamic_precision_int16"_test = [&] () { _test(GridFormat::int16); };
    "dynamic_precision_int32"_test = [&] () { _test(GridFormat::int32); };
    "dynamic_precision_int64"_test = [&] () { _test(GridFormat::int64); };
    "dynamic_precision_uint8"_test = [&] () { _test(GridFormat::uint8); };
    "dynamic_precision_uint16"_test = [&] () { _test(GridFormat::uint16); };
    "dynamic_precision_uint32"_test = [&] () { _test(GridFormat::uint32); };
    "dynamic_precision_uint64"_test = [&] () { _test(GridFormat::uint64); };

    return 0;
}
