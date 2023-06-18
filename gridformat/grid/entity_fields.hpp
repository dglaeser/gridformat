// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Grid
 * \brief Field implementations for data on grid entities.
 */
#ifndef GRIDFORMAT_GRID_ENTITY_FIELDS_HPP_
#define GRIDFORMAT_GRID_ENTITY_FIELDS_HPP_

#include <span>
#include <ranges>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <concepts>

#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/serialization.hpp>
#include <gridformat/common/flat_index_mapper.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/grid.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace EntityFieldsDetail {

    template<Concepts::Scalar ValueType>
    void fill_buffer(const Concepts::Scalar auto& value,
                    std::byte* buffer,
                    std::size_t& offset) {
        const auto cast_value = static_cast<ValueType>(value);
        std::copy_n(
            reinterpret_cast<const std::byte*>(&cast_value),
            sizeof(ValueType),
            buffer + offset
        );
        offset += sizeof(ValueType);
    }

    template<Concepts::Scalar ValueType>
    void fill_buffer(const std::ranges::range auto& r,
                    std::byte* buffer,
                    std::size_t& offset) {
        std::ranges::for_each(r, [&] (const auto& entry) {
            fill_buffer<ValueType>(entry, buffer, offset);
        });
    }

    template<typename ValueType,
             typename Grid,
             typename Entities,
             typename Extents,
             typename F>
    void fill_structured(const Grid& grid,
                         const Entities& entities,
                         const Extents& extents,
                         const F& field_function,
                         const MDLayout& layout,
                         Serialization& serialization) {
        auto values = serialization.as_span_of<ValueType>();
        const FlatIndexMapper index_mapper{extents};
        const auto values_offset = layout.dimension() == 1 ? 1 : layout.sub_layout(1).number_of_entries();
        std::ranges::for_each(entities, [&] (const auto& e) {
            const auto index = index_mapper.map(location(grid, e));
            const auto cur_offset = index*values_offset;
            auto cur_values = std::as_writable_bytes(values.subspan(cur_offset));

            std::size_t offset = 0;
            EntityFieldsDetail::fill_buffer<ValueType>(field_function(e), cur_values.data(), offset);
        });
    }

    template<bool is_point_data,
             typename ValueType,
             typename Grid,
             typename F>
    void fill_structured(const Grid& grid,
                         const F& field_function,
                         const MDLayout& layout,
                         Serialization& serialization) {
        if constexpr (Concepts::StructuredEntitySet<Grid>) {
            if constexpr (is_point_data)
                fill_structured<ValueType>(grid, points(grid), point_extents(grid), field_function, layout, serialization);
            else
                fill_structured<ValueType>(grid, cells(grid), extents(grid), field_function, layout, serialization);
        } else {
            throw TypeError("Only structured grids can be used for entity fields with structured grid ordering");
        }
    }

}  // namespace EntityFieldsDetail
#endif  // DOXYGEN


/*!
 * \ingroup Grid
 * \brief Field implementation for data on grid points.
 */
template<Concepts::Grid Grid,
         Concepts::PointFunction<Grid> FieldFunction,
         Concepts::Scalar ValueType = GridDetail::PointFunctionScalarType<Grid, FieldFunction>>
class PointField : public Field {
    using FieldType = GridDetail::PointFunctionValueType<Grid, FieldFunction>;

 public:
    explicit PointField(const Grid& grid,
                        FieldFunction&& field_function,
                        bool use_structured_grid_ordering,
                        const Precision<ValueType>& = {})
    : _grid{grid}
    , _field_function{std::move(field_function)}
    , _write_structured{use_structured_grid_ordering}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        return get_md_layout<FieldType>(number_of_points(_grid));
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        const auto layout = _layout();
        Serialization serialization(_size_in_bytes(layout));
        _fill(serialization, layout);
        return serialization;
    }

    void _fill(Serialization& serialization, const MDLayout& layout) const {
        if (_write_structured) {
            EntityFieldsDetail::fill_structured<true, ValueType>(_grid, _field_function, layout, serialization);
        } else {
            std::size_t offset = 0;
            std::byte* buffer = serialization.as_span().data();
            std::ranges::for_each(points(_grid), [&] (const auto& p) {
                EntityFieldsDetail::fill_buffer<ValueType>(_field_function(p), buffer, offset);
            });
        }
    }

    const Grid& _grid;
    FieldFunction _field_function;
    bool _write_structured;
};

/*!
 * \ingroup Grid
 * \brief Field implementation for data on grid cells.
 */
template<typename Grid,
         Concepts::CellFunction<Grid> FieldFunction,
         Concepts::Scalar ValueType = GridDetail::CellFunctionScalarType<Grid, FieldFunction>>
class CellField : public Field {
    using FieldType = GridDetail::CellFunctionValueType<Grid, FieldFunction>;

 public:
    explicit CellField(const Grid& grid,
                       FieldFunction&& field_function,
                       bool use_structured_grid_ordering,
                       const Precision<ValueType>& = {})
    : _grid{grid}
    , _field_function{std::move(field_function)}
    , _write_structured{use_structured_grid_ordering}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        return get_md_layout<FieldType>(number_of_cells(_grid));
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        const auto layout = _layout();
        Serialization serialization(_size_in_bytes(layout));
        _fill(serialization, layout);
        return serialization;
    }

    void _fill(Serialization& serialization, const MDLayout& layout) const {
        if (_write_structured) {
            EntityFieldsDetail::fill_structured<false, ValueType>(_grid, _field_function, layout, serialization);
        } else {
            std::size_t offset = 0;
            std::byte* buffer = serialization.as_span().data();
            std::ranges::for_each(cells(_grid), [&] (const auto& c) {
                EntityFieldsDetail::fill_buffer<ValueType>(_field_function(c), buffer, offset);
            });
        }
    }

    const Grid& _grid;
    FieldFunction _field_function;
    bool _write_structured;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_ENTITY_FIELDS_HPP_
