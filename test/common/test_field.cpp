// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <memory>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/field.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 private:
    std::vector<int> _values{1, 2, 3, 4};

    GridFormat::MDLayout _layout() const override {
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
    using GridFormat::Testing::eq;

    "field_layout"_test = [] () {
        std::unique_ptr<GridFormat::Field> field = std::make_unique<MyField>();
        expect(eq(field->layout().dimension(), 1_ul));
        expect(eq(field->layout().extent(0), 4_ul));
        expect(eq(field->precision().is_integral(), true));
        expect(eq(field->precision().is_signed(), true));
        expect(eq(field->precision().size_in_bytes(), sizeof(int)));
    };

    return 0;
}