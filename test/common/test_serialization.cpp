// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <ranges>
#include <vector>

#include <gridformat/common/serialization.hpp>

#include "../testing.hpp"

int main() {

    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;

    "serialization_push_back"_test = [] () {
        GridFormat::Serialization s;
        std::vector<int> v{1, 2, 3, 4};
        std::vector<std::byte> bytes{v.size()*sizeof(int)};
        std::ranges::copy(
            std::as_bytes(std::span{v}),
            bytes.begin()
        );
        s.push_back(std::move(bytes));
        expect(std::ranges::equal(s.template as_span_of<int>(), v));
    };

    "serialization_cut_front"_test = [] () {
        GridFormat::Serialization s;
        s.resize(4*sizeof(int));
        auto span = s.template as_span_of<int>();
        std::ranges::copy(std::vector{1, 2, 3, 4}, span.begin());
        s.cut_front(2*sizeof(int));
        expect(std::ranges::equal(s.template as_span_of<int>(), std::vector{3, 4}));
    };

    "serialization_cut_front_throws_on_exceeding_size"_test = [] () {
        GridFormat::Serialization s;
        s.resize(4);
        expect(throws<GridFormat::SizeError>([&] () { s.cut_front(5); }));
    };

    return 0;
}
