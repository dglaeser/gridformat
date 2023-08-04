// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <array>
#include <cmath>

#include <gridformat/common/field.hpp>
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

#ifndef DOXYGEN
namespace GridFormat::Encoding { struct Ascii; struct Base64; struct RawBinary; }
#endif  // DOXYGEN

namespace GridFormat::VTK {

//! \addtogroup VTK
//! \{

namespace DataFormat {

struct Inlined {};  //!< Inline data format (inside xml elements)
struct Appended {};  //!< Appended data format (all data is appended at the end of the xml file)

inline constexpr Inlined inlined;  //!< Instance of the inline data format
inline constexpr Appended appended;  //!< Instance of the appended data format

}  // namespace DataFormat

#ifndef DOXYGEN
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
        case (CellType::pixel): return 8;
        case (CellType::quadrilateral): return 9;
        case (CellType::polygon): return 7;
        case (CellType::tetrahedron): return 10;
        case (CellType::hexahedron): return 12;
        case (CellType::voxel): return 11;
        case (CellType::lagrange_segment): return 68;
        case (CellType::lagrange_triangle): return 69;
        case (CellType::lagrange_quadrilateral): return 70;
        case (CellType::lagrange_tetrahedron): return 71;
        case (CellType::lagrange_hexahedron): return 72;
    }

    throw NotImplemented("VTK cell type number for the given cell type");
}

inline constexpr CellType cell_type(std::uint8_t vtk_id) {
    switch (vtk_id) {
        case 1: return CellType::vertex;
        case 3: return CellType::segment;
        case 5: return CellType::triangle;
        case 8: return CellType::pixel;
        case 9: return CellType::quadrilateral;
        case 7: return CellType::polygon;
        case 10: return CellType::tetrahedron;
        case 12: return CellType::hexahedron;
        case 11: return CellType::voxel;
        case 68: return CellType::lagrange_segment;
        case 69: return CellType::lagrange_triangle;
        case 70: return CellType::lagrange_quadrilateral;
        case 71: return CellType::lagrange_tetrahedron;
        case 72: return CellType::lagrange_hexahedron;
    }

    throw NotImplemented("Cell type for the given VTK cell type number: " + std::to_string(vtk_id));
}

FieldPtr make_vtk_field(FieldPtr field) {
    const auto layout = field->layout();
    if (layout.dimension() < 2)
        return field;
    // (maybe) make vector/tensor fields 3d
    if (std::ranges::all_of(
            std::views::iota(std::size_t{1}, layout.dimension()),
            [&] (const std::size_t codim) { return layout.extent(codim) < 3; }
        ))
        return transform(field, FieldTransformation::extend_all_to(3));
    return field;
}

template<std::derived_from<Field> F>
    requires(!std::is_lvalue_reference_v<F>)
FieldPtr make_vtk_field(F&& field) {
    return make_vtk_field(make_field_ptr(std::forward<F>(field)));
}

template<typename ctype, GridDetail::ExposesPointRange Grid>
auto make_coordinates_field(const Grid& grid, bool structured_grid_ordering) {
    return make_vtk_field(PointField{
        grid,
        [&] (const auto& point) { return coordinates(grid, point); },
        structured_grid_ordering,
        Precision<ctype>{}
    });
}

template<typename HeaderType = std::size_t,
         Concepts::UnstructuredGrid Grid,
         std::ranges::forward_range Cells,
         typename PointMap>
    requires(std::is_lvalue_reference_v<PointMap>)
auto make_connectivity_field(const Grid& grid,
                             Cells&& cells,
                             PointMap&& map) {
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
        LVReferenceOrValue<Cells> _cells;
        LVReferenceOrValue<PointMap> _point_map;
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
        LVReferenceOrValue<Cells> _cells;
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
        },
        false
    });
}

inline auto active_array_attribute_for_rank(unsigned int rank) {
    if (rank > 2)
        throw ValueError("Rank must be <= 2");
    static constexpr std::array attributes{"Scalars", "Vectors", "Tensors"};
    return attributes[rank];
}

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
    std::string extents_string(const R1& from, const R2& to) {
        static_assert(static_size<R1> == static_size<R2>);
        static_assert(static_size<R1> > 0);

        std::string result;
        std::ranges::for_each(from, [&, i=0] (const auto& ex0) mutable {
            result += as_string(ex0) + " " + as_string(Ranges::at(i++, to)) + " ";
        });
        for (unsigned int i = static_size<R1>; i < 3; ++i)
            result += " 0 0 ";
        result.pop_back();
        return result;
    }

    template<Concepts::StructuredEntitySet Grid>
        requires(!Concepts::StaticallySizedRange<Grid>)
    std::string extents_string(const Grid& grid) {
        return extents_string(extents(grid));
    }

    template<Concepts::StaticallySizedRange R1,
             Concepts::StaticallySizedRange R2>
    std::array<std::size_t, 6> get_extents(const R1& r1, const R2& r2) {
        static_assert(static_size<R1> == static_size<R2>);
        int i = 0;
        std::array<std::size_t, 6> result;
        std::ranges::fill(result, 0);

        auto it1 = std::ranges::begin(r1);
        auto it2 = std::ranges::begin(r2);
        for (; it1 != std::ranges::end(r1); ++it1, ++it2, ++i) {
            result[i*2 + 0] = *it1;
            result[i*2 + 1] = *it2;
        }
        return result;
    }

    template<Concepts::StaticallySizedRange R>
    std::array<std::size_t, 6> get_extents(const R& r1) {
        std::array<std::size_t, static_size<R>> origin;
        std::ranges::fill(origin, 0);
        return get_extents(origin, r1);
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

    std::size_t number_of_entities(const std::array<std::size_t, 6>& extents) {
        return std::max(extents[1] - extents[0], std::size_t{1})
                *std::max(extents[3] - extents[2], std::size_t{1})
                *std::max(extents[5] - extents[4], std::size_t{1});
    }

}  // namespace CommonDetail
#endif  // DOXYGEN

//! \} group VTK
}  // namespace GridFormat::VTK


#endif  // GRIDFORMAT_VTK_COMMON_HPP_
