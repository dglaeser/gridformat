// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <vector>
#include <memory>
#include <ranges>
#include <algorithm>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/field.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 public:
    MyField() = default;
    MyField(bool produce_size_mismatch) : _size_mismatch{produce_size_mismatch} {}

 private:
    std::vector<int> _values{1, 2, 3, 4};
    bool _size_mismatch = false;

    GridFormat::MDLayout _layout() const override {
        if (_size_mismatch)
            return GridFormat::MDLayout{{_values.size() + 1}};
        return GridFormat::MDLayout{{_values.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<int>{}};
    }

    GridFormat::Serialization _serialized() const override {
        GridFormat::Serialization result(_values.size()*sizeof(int));
        std::ranges::copy(
            std::as_bytes(std::span{_values}),
            result.as_span().begin()
        );
        return result;
    }
};

int main() {

    using namespace GridFormat::Testing::Literals;
    using GridFormat::Testing::operator""_test;
    using GridFormat::Testing::expect;
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    "field_layout"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        expect(eq(field->layout().dimension(), 1_ul));
        expect(eq(field->layout().extent(0), 4_ul));
        expect(eq(field->precision().is_integral(), true));
        expect(eq(field->precision().is_signed(), true));
        expect(eq(field->precision().size_in_bytes(), sizeof(int)));
    };

    "field_layout_mismatch_throws_upon_serialization"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>(true);
        expect(eq(field->layout().dimension(), 1_ul));
        expect(eq(field->layout().extent(0), 5_ul));
        expect(throws<GridFormat::SizeError>([&] () { field->serialized(); }));
    };

    "field_export_throws_when_size_does_not_match"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        auto values = field->template export_to<std::vector<int>>();
        expect(std::ranges::equal(values, std::vector{1, 2, 3, 4}));

        values.resize(values.size() - 1);
        expect(throws<GridFormat::SizeError>([&] () { field->export_to(values); }));

        values.resize(7);
        std::ranges::fill(values, 42);
        field->export_to(values);
        expect(std::ranges::equal(values | std::views::take(4), std::vector{1, 2, 3, 4}));
        expect(std::ranges::equal(values | std::views::drop(4), std::vector{42, 42, 42}));
    };

    return 0;
}
