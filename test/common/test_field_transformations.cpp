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
#include <gridformat/common/scalar_field.hpp>
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

class ZeroField : public GridFormat::Field {
 private:
    GridFormat::MDLayout _layout() const override { return GridFormat::MDLayout{}; }
    GridFormat::DynamicPrecision _precision() const override { return GridFormat::Precision<int>{}; }
    GridFormat::Serialization _serialized() const override { return GridFormat::Serialization{}; }
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
        auto s = transformed_field.serialized();
        expect(std::ranges::equal(s.template as_span_of<int>(), std::vector{1, 2, 3, 4}));
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

    "transformed_reshaped_field"_test = [] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<std::array<int, 2>>{{2, 3}, {2, 3}},
            GridFormat::Precision<double>{}
        });
        auto reshaped = GridFormat::ReshapedField{field_ptr, GridFormat::MDLayout{{4, 1}}};
        expect(eq(reshaped.layout().dimension(), 2_ul));
        expect(eq(reshaped.layout().extent(0), 4_ul));
        expect(eq(reshaped.layout().extent(1), 1_ul));
        auto serialized = reshaped.serialized();
        expect(std::ranges::equal(serialized.template as_span_of<double>(), std::vector{2., 3., 2., 3.}));
    };

    "transformed_reshaped_field_throws_upon_layout_mismatch"_test = [] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<std::array<int, 2>>{{2, 3}, {2, 3}},
            GridFormat::Precision<double>{}
        });
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::ReshapedField{field_ptr, GridFormat::MDLayout{{5, 1}}};
        }));
    };

    "merged_scalar_fields"_test = [] () {
        GridFormat::MergedField merged{
            GridFormat::make_field_ptr(GridFormat::ScalarField{42}),
            GridFormat::make_field_ptr(GridFormat::ScalarField{43})
        };
        expect(eq(merged.layout().dimension(), 1_ul));
        expect(eq(merged.layout().extent(0), 2_ul));
        expect(std::ranges::equal(
            merged.serialized().template as_span_of<int>(),
            std::vector<int>{42, 43}
        ));
    };

    "merged_scalar_fields_from_vec"_test = [] () {
        GridFormat::MergedField merged{
            GridFormat::make_field_ptr(GridFormat::ScalarField{42}),
            GridFormat::make_field_ptr(RangeField{std::vector<int>{43}})
        };
        expect(eq(merged.layout().dimension(), 1_ul));
        expect(eq(merged.layout().extent(0), 2_ul));
        expect(std::ranges::equal(
            merged.serialized().template as_span_of<int>(),
            std::vector<int>{42, 43}
        ));
    };

    "merged_2d_fields"_test = [] () {
        GridFormat::MergedField merged{
            GridFormat::make_field_ptr(RangeField{std::vector<std::array<int, 1>>{{42}}}),
            GridFormat::make_field_ptr(RangeField{std::vector<std::array<int, 1>>{{43}}})
        };
        expect(eq(merged.layout().dimension(), 2_ul));
        expect(eq(merged.layout().extent(0), 2_ul));
        expect(eq(merged.layout().extent(1), 1_ul));
        expect(std::ranges::equal(
            merged.serialized().template as_span_of<int>(),
            std::vector<int>{42, 43}
        ));
    };

    "merged_fields_throw_with_non_matching_layouts"_test = [] () {
        expect(throws<GridFormat::ValueError>([] () {
            GridFormat::MergedField merged{
                GridFormat::make_field_ptr(RangeField{std::vector<int>{42}}),
                GridFormat::make_field_ptr(RangeField{std::vector<std::array<int, 1>>{{43}}})
            };
        }));
    };

    "merged_fields_throw_with_non_matching_precision"_test = [] () {
        expect(throws<GridFormat::ValueError>([] () {
            GridFormat::MergedField merged{
                GridFormat::make_field_ptr(RangeField{std::vector<double>{42}}),
                GridFormat::make_field_ptr(RangeField{std::vector<int>{43}})
            };
        }));
    };

    "merged_fields_throw_with_zero_dimension"_test = [] () {
        expect(throws<GridFormat::ValueError>([] () {
            GridFormat::MergedField merged{GridFormat::make_field_ptr(ZeroField{})};
        }));
    };

    "sliced_field"_test = [&] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<std::array<int, 2>>{
                {2, 42},
                {2, 43}
            },
            GridFormat::Precision<double>{}
        });
        GridFormat::SlicedField sliced{field_ptr, {
            .from = {0, 1},
            .to = {2, 2}
        }};
        auto serialization = sliced.serialized();
        expect(std::ranges::equal(serialization.template as_span_of<double>(), std::vector{42., 43.}));
    };

    "sliced_field_dimension_mismatch_throws"_test = [&] () {
        auto field_ptr = GridFormat::make_field_ptr(RangeField{
            std::vector<std::array<int, 2>>{{2, 42}, {2, 43}}
        });
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::SlicedField f{field_ptr, {.from = {0, 0}, .to = {2}}};
            f.layout();
        }));
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::SlicedField f{field_ptr, {.from = {0}, .to = {2}}};
            f.layout();
        }));
        expect(throws<GridFormat::SizeError>([&] () {
            GridFormat::SlicedField f{field_ptr, {.from = {0, 0, 0}, .to = {1, 1, 1}}};
            f.layout();
        }));
    };

    return 0;
}
