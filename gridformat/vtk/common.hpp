// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for VTK writers
 */
#ifndef GRIDFORMAT_VTK_COMMON_HPP_
#define GRIDFORMAT_VTK_COMMON_HPP_

#include <ranges>
#include <cassert>
#include <utility>
#include <type_traits>
#include <algorithm>

#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/matrix.hpp>
#include <gridformat/common/type_traits.hpp>
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/entity_fields.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/grid.hpp>

// forward declarations
namespace GridFormat::Encoding { struct Ascii; struct Base64; struct RawBinary; }
// end forward declarations

namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

namespace DataFormat {

struct Inlined {};
struct Appended {};

inline constexpr Inlined inlined;
inline constexpr Appended appended;

}  // namespace DataFormat

namespace Traits {

template<typename T> struct ProducesValidXML;
template<> struct ProducesValidXML<Encoding::Ascii> : public std::true_type {};
template<> struct ProducesValidXML<Encoding::Base64> : public std::true_type {};
template<> struct ProducesValidXML<Encoding::RawBinary> : public std::false_type {};

}  // namespace Traits

template<typename Encoder>
inline constexpr bool produces_valid_xml(const Encoder&) {
    static_assert(
        is_complete<Traits::ProducesValidXML<Encoder>>,
        "Traits::ProducesValidXML was not specialized for the given encoder"
    );
    return Traits::ProducesValidXML<Encoder>::value;
}

inline constexpr std::uint8_t cell_type_number(CellType t) {
    switch (t) {
        case (CellType::vertex): return 1;
        case (CellType::segment): return 3;
        case (CellType::triangle): return 5;
        case (CellType::rectangle): return 8;
        case (CellType::quadrilateral): return 9;
        case (CellType::polygon): return 7;
        case (CellType::tetrahedron): return 10;
        case (CellType::hexahedron): return 12;
    }

    throw NotImplemented("VTK cell type number for the given cell type");
}

FieldPtr make_vtk_field(FieldPtr field) {
    // vector/tensor fields must be made 3d
    if (field->layout().dimension() > 1)
        return FieldTransformation::extend_all_to(3)(field);
    return field;
}

template<std::derived_from<Field> F> requires (!std::is_lvalue_reference_v<F>)
FieldPtr make_vtk_field(F&& field) {
    return make_vtk_field(make_shared(std::forward<F>(field)));
}

template<typename ctype, GridDetail::ExposesPointRange Grid>
auto make_coordinates_field(const Grid& grid) {
    return make_vtk_field(PointField{
        grid,
        [&] (const auto& point) { return coordinates(grid, point); },
        Precision<ctype>{}
    });
}

template<typename HeaderType = std::size_t,
         Concepts::UnstructuredGrid Grid,
         std::ranges::forward_range Cells,
         typename PointMap>
auto make_connectivity_field(const Grid& grid,
                             Cells&& cells,
                             PointMap&& map)
requires(std::is_lvalue_reference_v<PointMap>)
{
    class ConnectivityField : public Field {
     public:
        explicit ConnectivityField(const Grid& g,
                                   Cells&& cells,
                                   PointMap&& map)
        : _grid(g)
        , _cells{std::forward<Cells>(cells)}
        , _point_map{std::forward<PointMap>(map)} {
            _num_values = 0;
            std::ranges::for_each(_cells, [&] (const auto& cell) {
                _num_values += number_of_points(_grid, cell);
            });
        }

     private:
        MDLayout _layout() const override { return MDLayout{{_num_values}}; }
        DynamicPrecision _precision() const override { return Precision<HeaderType>{}; }
        Serialization _serialized() const override {
            Serialization serialization(sizeof(HeaderType)*_num_values);
            HeaderType* data = serialization.as_span_of<HeaderType>().data();

            std::size_t i = 0;
            std::ranges::for_each(_cells, [&] (const auto& cell) {
                std::ranges::for_each(points(_grid, cell), [&] (const auto& point) {
                    data[i] = _point_map.at(id(_grid, point));
                    i++;
                });
            });
            return serialization;
        }

        const Grid& _grid;
        std::conditional_t<std::is_lvalue_reference_v<Cells>, Cells&, std::decay_t<Cells>> _cells;
        std::conditional_t<std::is_lvalue_reference_v<PointMap>, PointMap&, std::decay_t<PointMap>> _point_map;
        HeaderType _num_values;
    } _field{grid, std::forward<Cells>(cells), std::forward<PointMap>(map)};

    return make_vtk_field(std::move(_field));
}

template<typename HeaderType = std::size_t,
         Concepts::UnstructuredGrid Grid,
         typename PointMap>
auto make_connectivity_field(const Grid& grid, PointMap&& map) {
    return make_connectivity_field<HeaderType>(grid, cells(grid), std::forward<PointMap>(map));
}

template<typename HeaderType = std::size_t,
         Concepts::UnstructuredGrid Grid,
         std::ranges::range Cells>
auto make_offsets_field(const Grid& grid, Cells&& cells) {
    class OffsetField : public Field {
     public:
        explicit OffsetField(const Grid& g, Cells&& cells)
        : _grid(g)
        , _cells{std::forward<Cells>(cells)}
        , _num_cells{static_cast<HeaderType>(Ranges::size(_cells))}
        {}

     private:
        MDLayout _layout() const override { return MDLayout{{_num_cells}}; }
        DynamicPrecision _precision() const override { return Precision<HeaderType>{}; }
        Serialization _serialized() const override {
            Serialization serialization(sizeof(HeaderType)*_num_cells);
            HeaderType* data = serialization.as_span_of<HeaderType>().data();

            std::size_t i = 0;
            HeaderType offset = 0;
            std::ranges::for_each(_cells, [&] (const auto& cell) {
                offset += number_of_points(_grid, cell);
                data[i] = offset;
                i++;
            });
            return serialization;
        }

        const Grid& _grid;
        std::conditional_t<std::is_lvalue_reference_v<Cells>, Cells&, std::decay_t<Cells>> _cells;
        HeaderType _num_cells;
    } _field{grid, std::forward<Cells>(cells)};

    return make_vtk_field(std::move(_field));
}

template<typename HeaderType = std::size_t, Concepts::UnstructuredGrid Grid>
auto make_offsets_field(const Grid& grid) {
    return make_offsets_field<HeaderType>(grid, cells(grid));
}

template<Concepts::UnstructuredGrid Grid>
auto make_cell_types_field(const Grid& grid) {
    return make_vtk_field(CellField{
        grid,
        [&] (const auto& cell) {
            return VTK::cell_type_number(type(grid, cell));
        }
    });
}

#ifndef DOXYGEN
namespace CommonDetail {

    template<Concepts::StaticallySizedRange R>
    std::string number_string_3d(const R& r) {
        static_assert(static_size<R> >= 1 || static_size<R> <= 3);
        if constexpr (static_size<R> == 3)
            return as_string(r);
        if constexpr (static_size<R> == 2)
            return as_string(r) + " 0";
        if constexpr (static_size<R> == 1)
            return as_string(r) + " 0 0";
    }

    template<Concepts::StaticallySizedMDRange<2> R>
    std::string direction_string(const R& basis) {
        Matrix m{basis};
        m.transpose();

        std::string result = "";
        std::ranges::for_each(m, [&] (const auto& row) {
            if (result != "")
                result += " ";
            result += number_string_3d(row);
        });
        for (int i = static_size<R>; i < 3; ++i)
            result += " 0 0 0";
        return result;
    }

    template<Concepts::StaticallySizedRange R>
    std::string extents_string(const R& r) {
        int i = 0;
        std::string result;
        std::ranges::for_each(r, [&] (const auto& entry) {
            result += (i > 0 ? " 0 " : "0 ") + as_string(entry);
            ++i;
        });
        for (i = static_size<R>; i < 3; ++i)
            result += " 0 0";
        return result;
    }

    template<Concepts::StaticallySizedRange R1,
             Concepts::StaticallySizedRange R2>
    std::string extents_string(const R1& r1, const R2& r2) {
        static_assert(static_size<R1> == static_size<R2>);
        int i = 0;
        std::string result;
        auto it1 = std::ranges::begin(r1);
        auto it2 = std::ranges::begin(r2);
        for (; it1 != std::ranges::end(r1); ++it1, ++it2, ++i)
            result += (i > 0 ? " " : "")
                        + as_string(*it1) + " "
                        + as_string(*it2);
        for (i = static_size<R1>; i < 3; ++i)
            result += " 0 0";
        return result;
    }

    template<Concepts::StructuredGrid Grid>
    std::string extents_string(const Grid& grid) {
        return extents_string(extents(grid));
    }

    template<Concepts::StaticallySizedRange Spacing>
    auto structured_grid_axis_orientation(const Spacing& spacing) {
        std::array<bool, static_size<Spacing>> result;
        std::ranges::copy(
            spacing | std::views::transform([&] <typename CT> (const CT dx) { return dx <= CT{0}; }),
            result.begin()
        );
        return result;
    }

}  // namespace CommonDetail
#endif  // DOXYGEN

//! \} group VTK
}  // namespace GridFormat::VTK


#endif  // GRIDFORMAT_VTK_COMMON_HPP_
