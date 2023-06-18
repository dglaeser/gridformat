// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

#include <array>
#include <vector>
#include <memory>
#include <ranges>
#include <utility>
#include <algorithm>
#include <type_traits>

#include <gridformat/common/serialization.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/field_transformations.hpp>

#include "../testing.hpp"

template<std::ranges::forward_range R, typename ValueType = GridFormat::MDRangeScalar<R>>
class RangeField : public GridFormat::Field {
    static constexpr bool use_range_value_type =
        std::is_same_v<std::ranges::range_value_t<std::decay_t<R>>, ValueType>;

 public:
    explicit RangeField(R&& range, const GridFormat::Precision<ValueType>& = {})
    : _range{std::move(range)}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const GridFormat::MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    GridFormat::MDLayout _layout() const override {
        return GridFormat::get_md_layout(_range);
    }

    GridFormat::DynamicPrecision _precision() const override {
        return {GridFormat::Precision<ValueType>{}};
    }

    GridFormat::Serialization _serialized() const override {
        const auto layout = GridFormat::get_md_layout(_range);
        GridFormat::Serialization serialization(_size_in_bytes(layout));
        _fill(serialization);
        return serialization;
    }

    void _fill(GridFormat::Serialization& serialization) const {
        std::size_t offset = 0;
        _fill_buffer(_range, serialization.as_span().data(), offset);
    }

    void _fill_buffer(const std::ranges::range auto& r,
                      std::byte* serialization,
                      std::size_t& offset) const {
        std::ranges::for_each(r, [&] (const auto& entry) {
            _fill_buffer(entry, serialization, offset);
        });
    }

    void _fill_buffer(const GridFormat::Concepts::Scalar auto& value,
                      std::byte* serialization,
                      std::size_t& offset) const {
        const auto cast_value = static_cast<ValueType>(value);
        std::copy_n(
            reinterpret_cast<const std::byte*>(&cast_value),
            sizeof(ValueType),
            serialization + offset
        );
        offset += sizeof(ValueType);
    }

    R _range;
};

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
    using GridFormat::Testing::throws;
    using GridFormat::Testing::eq;

    using GridFormat::TransformedField;
    using GridFormat::ExtendedField;
    using GridFormat::FieldTransformation::identity;
    using GridFormat::FieldTransformation::extend_to;
    using GridFormat::FieldTransformation::extend_all_to;

    "transformed_field_identity"_test = [] () {
        GridFormat::TransformedField transformed{GridFormat::make_field_ptr(MyField{}), identity};
        expect(eq(transformed.layout().dimension(), 1_ul));
        expect(eq(transformed.layout().extent(0), 4_ul));
        expect(eq(transformed.precision().is_integral(), true));
        expect(eq(transformed.precision().is_signed(), true));
        expect(eq(transformed.precision().size_in_bytes(), sizeof(int)));
    };

    "transformed_field_identity_identity"_test = [] () {
        TransformedField transformed_field{identity(GridFormat::make_field_ptr(MyField{})), identity};
        expect(eq(transformed_field.layout().dimension(), 1_ul));
        expect(eq(transformed_field.layout().extent(0), 4_ul));
        expect(eq(transformed_field.precision().is_integral(), true));
        expect(eq(transformed_field.precision().is_signed(), true));
        expect(eq(transformed_field.precision().size_in_bytes(), sizeof(int)));
    };

    "transformed_field_extend"_test = [] () {
        auto field_ptr = GridFormat::make_field_ptr(
            RangeField{
                std::vector<std::array<int, 2>>{{2, 3}, {4, 5}},
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
        auto field_ptr = GridFormat::make_field_ptr(
            RangeField{
                std::vector<std::array<int, 2>>{{2, 3}, {4, 5}},
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
        auto field_ptr = GridFormat::make_field_ptr(
            RangeField{
                std::vector<std::array<int, 2>>{{2, 3}, {4, 5}},
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

    "transformed_field_extend_1d_throws"_test = [] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<int>{2, 3}, GridFormat::Precision<double>{}
        });
        expect(throws<GridFormat::SizeError>([&] () { ExtendedField{field_ptr, GridFormat::MDLayout{{3}}}.layout(); }));
        expect(throws<GridFormat::SizeError>([&] () { TransformedField{field_ptr, extend_to(GridFormat::MDLayout{{3}})}; }));
    };

    "transformed_field_extend_layout_mismatch_throws"_test = [] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<std::array<int, 2>>{{2, 3}, {2, 3}},
            GridFormat::Precision<double>{}
        });
        expect(throws<GridFormat::SizeError>([&] () { ExtendedField{field_ptr, GridFormat::MDLayout{{3, 3}}}.layout(); }));
        expect(throws<GridFormat::SizeError>([&] () { TransformedField{field_ptr, extend_to(GridFormat::MDLayout{{3, 3}})}.layout(); }));
        TransformedField{field_ptr, extend_to(GridFormat::MDLayout{{4}})}.layout();
    };

    return 0;
}
