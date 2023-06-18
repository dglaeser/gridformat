// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <algorithm>
#include <iostream>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/enumerated_range.hpp>
#include "../testing.hpp"


auto make_test_vector() {
    return std::vector{42, 43, 44, 45, 46};
}

int main() {
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;

    "enumerated_range_by_value"_test = [] () {
        auto enumerated = GridFormat::Ranges::enumerated(make_test_vector());
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<1>(pair); }),
            make_test_vector()
        ));
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<0>(pair); }),
            std::views::iota(std::size_t{0}, make_test_vector().size())
        ));
    };

    "enumerated_range_by_const_ref"_test = [] () {
        const auto data = std::vector{42, 43, 44, 45, 46};
        auto enumerated = GridFormat::Ranges::enumerated(data);
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<1>(pair); }),
            data
        ));
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<0>(pair); }),
            std::views::iota(std::size_t{0}, data.size())
        ));
    };

    "enumerated_range_by_non_const_ref"_test = [] () {
        auto data = std::vector{42, 43, 44, 45, 46};
        auto enumerated = GridFormat::Ranges::enumerated(data);
        std::ranges::for_each(enumerated, [] (auto pair) {
            std::get<1>(pair) += 1;
        });
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<1>(pair); }),
            GridFormat::Ranges::incremented(make_test_vector(), 1)
        ));
        expect(std::ranges::equal(
            enumerated | std::views::transform([] (const auto& pair) { return std::get<0>(pair); }),
            std::views::iota(std::size_t{0}, data.size())
        ));
    };

    return 0;
}
