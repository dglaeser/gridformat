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

    "field_export_return_value"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        std::vector<int> exported(4);
        decltype(auto) from_lvalue = field->export_to(exported);
        decltype(auto) from_rvalue = field->export_to(std::vector<int>{});
        expect(std::ranges::equal(from_lvalue, std::vector{1, 2, 3, 4}));
        expect(std::ranges::equal(from_rvalue, std::vector{1, 2, 3, 4}));
        static_assert(std::is_same_v<decltype(from_lvalue), std::vector<int>&>);
        static_assert(std::is_same_v<decltype(from_rvalue), std::vector<int>&&>);
    };

    "field_export_to_vector"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        using V = std::vector<int>;
        std::vector<V> exported{};
        exported.push_back(field->template export_to<V>());
        exported.push_back(field->export_to(V{}));
        { V r(4); field->export_to(r); exported.push_back(r); }
        for (const auto& ex : exported) {
            expect(eq(ex.size(), std::size_t{4}));
            expect(std::ranges::equal(ex, std::vector{1, 2, 3, 4}));
        }
    };

    "field_export_throws_when_size_does_not_match"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        auto values = field->template export_to<std::vector<int>>();
        expect(std::ranges::equal(values, std::vector{1, 2, 3, 4}));

        values.resize(values.size() - 1);
        expect(throws<GridFormat::SizeError>([&] () {
            field->export_to(values, GridFormat::Field::no_resize);
        }));

        values.resize(7);
        std::ranges::fill(values, 42);
        field->export_to(values, GridFormat::Field::no_resize);
        expect(std::ranges::equal(values | std::views::take(4), std::vector{1, 2, 3, 4}));
        expect(std::ranges::equal(values | std::views::drop(4), std::vector{42, 42, 42}));
    };

    "field_template_export_throws_when_size_not_interoperable"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        auto values = field->template export_to<std::vector<std::array<int, 2>>>();
        expect(std::ranges::equal(values[0], std::vector{1, 2}));
        expect(std::ranges::equal(values[1], std::vector{3, 4}));

        expect(throws<GridFormat::TypeError>([&] () { field->template export_to<std::vector<std::array<int, 3>>>(); }));
    };

    "field_export_to_scalar_throws_for_non_scalar_field"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();

        int value;
        expect(throws<GridFormat::TypeError>([&] () { field->export_to(value); }));
        expect(throws<GridFormat::TypeError>([&] () { field->template export_to<int>(); }));
    };

    return 0;
}
