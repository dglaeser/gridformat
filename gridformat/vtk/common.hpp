// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for VTK writers
 */
#ifndef GRIDFORMAT_VTK_COMMON_HPP_
#define GRIDFORMAT_VTK_COMMON_HPP_

#include <cassert>
#include <utility>
#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/accumulated_range.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/range_field.hpp>
#include <gridformat/common/flat_field.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/entity_fields.hpp>
#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/grid.hpp>

// forward declarations
namespace GridFormat::Encoding { struct Ascii; class AsciiWithOptions; struct Base64; struct RawBinary; }
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
template<> struct ProducesValidXML<Encoding::AsciiWithOptions> : public std::true_type {};
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

template<typename ctype, typename Grid> requires(GridDetail::exposes_point_range<Grid>)
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
    return make_vtk_field(
        FlatField{
            std::forward<Cells>(cells)
                | std::views::transform([&] (const auto& cell) {
                    return points(grid, cell)
                        | std::views::transform([&] (const auto& point) {
                            return map.at(id(grid, point));
                        });
        }),
        Precision<HeaderType>{}
    });
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
    return make_vtk_field(RangeField{
        AccumulatedRange{
            std::forward<Cells>(cells)
            | std::views::transform([&] (const Cell<Grid>& cell) {
                return Ranges::size(points(grid, cell));
            })
        },
        Precision<HeaderType>{}
    });
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

//! \} group VTK

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_COMMON_HPP_
