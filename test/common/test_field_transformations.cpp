// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <memory>

#include <gridformat/common/range_field.hpp>
#include <gridformat/common/field_transformations.hpp>

#include "../testing.hpp"

class MyField : public GridFormat::Field {
 private:
    std::vector<int> _values{1, 2, 3, 4};

    GridFormat::MDLayout _layout() const override {
        return GridFormat::MDLayout{std::vector<std::size_t>{_values.size()}};
    }

    GridFormat::DynamicPrecision _precision() const override {
        return GridFormat::Precision<int>{};
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

    using GridFormat::TransformedField;
    using GridFormat::FieldTransformation::identity;
    using GridFormat::FieldTransformation::extend_to;
    using GridFormat::FieldTransformation::extend_all_to;

    "transformed_field_identity"_test = [] () {
        GridFormat::TransformedField transformed{GridFormat::make_shared(MyField{}), identity};
        expect(eq(transformed.layout().dimension(), 1_ul));
        expect(eq(transformed.layout().extent(0), 4_ul));
        expect(eq(transformed.precision().is_integral(), true));
        expect(eq(transformed.precision().is_signed(), true));
        expect(eq(transformed.precision().size_in_bytes(), sizeof(int)));
    };

    "transformed_field_identity_identity"_test = [] () {
        TransformedField transformed_field{identity(GridFormat::make_shared(MyField{})), identity};
        expect(eq(transformed_field.layout().dimension(), 1_ul));
        expect(eq(transformed_field.layout().extent(0), 4_ul));
        expect(eq(transformed_field.precision().is_integral(), true));
        expect(eq(transformed_field.precision().is_signed(), true));
        expect(eq(transformed_field.precision().size_in_bytes(), sizeof(int)));
    };

    "transformed_field_extend"_test = [] () {
        auto field_ptr = GridFormat::make_shared(
            GridFormat::RangeField{
                std::vector<std::vector<int>>{{2, 3}, {4, 5}},
                GridFormat::Precision<double>{}
            }
        );
        TransformedField extended{field_ptr, extend_to(GridFormat::MDLayout{{3}})};
        expect(eq(extended.layout().dimension(), 2_ul));
        expect(eq(extended.layout().extent(0), 2_ul));
        expect(eq(extended.layout().extent(1), 3_ul));
        expect(eq(extended.precision().is_integral(), false));
        expect(eq(extended.precision().is_signed(), true));
        expect(eq(extended.precision().size_in_bytes(), sizeof(double)));
        expect(std::ranges::equal(
            extended.serialized().template as_span_of<double>(),
            std::vector<double>{2, 3, 0, 4, 5, 0}
        ));
    };

    "transformed_field_extend_all"_test = [] () {
        auto field_ptr = GridFormat::make_shared(
            GridFormat::RangeField{
                std::vector<std::vector<int>>{{2, 3}, {4, 5}},
                GridFormat::Precision<double>{}
            }
        );
        TransformedField field_3d{field_ptr, extend_all_to(3)};
        expect(eq(field_3d.layout().dimension(), 2_ul));
        expect(eq(field_3d.layout().extent(0), 2_ul));
        expect(eq(field_3d.precision().is_integral(), false));
        expect(eq(field_3d.precision().is_signed(), true));
        expect(eq(field_3d.precision().size_in_bytes(), sizeof(double)));
        expect(std::ranges::equal(
            field_3d.serialized().template as_span_of<double>(),
            std::vector<double>{2, 3, 0, 4, 5, 0}
        ));
    };

    "transformed_field_extend_flatten"_test = [] () {
        auto field_ptr = GridFormat::make_shared(
            GridFormat::RangeField{
                std::vector<std::vector<int>>{{2, 3}, {4, 5}},
                GridFormat::Precision<double>{}
            }
        );
        TransformedField flattened{
            extend_to(GridFormat::MDLayout{{3}})(field_ptr),
            GridFormat::FieldTransformation::flatten
        };
        expect(eq(flattened.layout().dimension(), 1_ul));
        expect(eq(flattened.layout().extent(0), 6_ul));
        expect(eq(flattened.precision().is_integral(), false));
        expect(eq(flattened.precision().is_signed(), true));
        expect(eq(flattened.precision().size_in_bytes(), sizeof(double)));
        expect(std::ranges::equal(
            flattened.serialized().template as_span_of<double>(),
            std::vector<double>{2, 3, 0, 4, 5, 0}
        ));
    };

    return 0;
}
