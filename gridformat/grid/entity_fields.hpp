// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <gridformat/common/serialization.hpp>
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
 public:
    explicit PointField(const Grid& grid,
                        FieldFunction&& field_function,
                        const Precision<ValueType>& = {})
    : _grid{grid}
    , _field_function{std::move(field_function)}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        const auto& first_point = *std::ranges::begin(points(_grid));
        const auto sub_layout = get_md_layout(_field_function(first_point));
        if (sub_layout.is_scalar())
            return MDLayout{{number_of_points(_grid)}};

        std::vector<std::size_t> extents(sub_layout.dimension() + 1);
        extents[0] = number_of_points(_grid);
        for (std::size_t i = 0; i < sub_layout.dimension(); ++i)
            extents[i+1] = sub_layout.extent(i);
        return MDLayout{extents};
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        const auto layout = _layout();
        Serialization serialization(_size_in_bytes(layout));
        _fill(serialization);
        return serialization;
    }

    void _fill(Serialization& serialization) const {
        std::size_t offset = 0;
        std::byte* buffer = serialization.as_span().data();
        std::ranges::for_each(points(_grid), [&] (const auto& p) {
            EntityFieldsDetail::fill_buffer<ValueType>(_field_function(p), buffer, offset);
        });
    }

    const Grid& _grid;
    FieldFunction _field_function;
};

/*!
 * \ingroup Grid
 * \brief Field implementation for data on grid cells.
 */
template<typename Grid,
         Concepts::CellFunction<Grid> FieldFunction,
         Concepts::Scalar ValueType = GridDetail::CellFunctionScalarType<Grid, FieldFunction>>
class CellField : public Field {
 public:
    explicit CellField(const Grid& grid,
                       FieldFunction&& field_function,
                       const Precision<ValueType>& = {})
    : _grid{grid}
    , _field_function{std::move(field_function)}
    {}

 private:
    std::size_t _size_in_bytes() const {
        return _size_in_bytes(_layout());
    }

    std::size_t _size_in_bytes(const MDLayout& layout) const {
        return layout.number_of_entries()*sizeof(ValueType);
    }

    MDLayout _layout() const override {
        const auto& first_cell = *std::ranges::begin(cells(_grid));
        const auto sub_layout = get_md_layout(_field_function(first_cell));
        if (sub_layout.is_scalar())
            return MDLayout{{number_of_cells(_grid)}};

        std::vector<std::size_t> extents(sub_layout.dimension() + 1);
        extents[0] = number_of_cells(_grid);
        for (std::size_t i = 0; i < sub_layout.dimension(); ++i)
            extents[i+1] = sub_layout.extent(i);
        return MDLayout{extents};
    }

    DynamicPrecision _precision() const override {
        return DynamicPrecision{Precision<ValueType>{}};
    }

    Serialization _serialized() const override {
        const auto layout = _layout();
        Serialization serialization(_size_in_bytes(layout));
        _fill(serialization);
        return serialization;
    }

    void _fill(Serialization& serialization) const {
        std::size_t offset = 0;
        std::byte* buffer = serialization.as_span().data();
        std::ranges::for_each(cells(_grid), [&] (const auto& c) {
            EntityFieldsDetail::fill_buffer<ValueType>(_field_function(c), buffer, offset);
        });
    }

    const Grid& _grid;
    FieldFunction _field_function;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_GRID_ENTITY_FIELDS_HPP_
