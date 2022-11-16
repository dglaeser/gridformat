// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for VTK writers
 */
#ifndef GRIDFORMAT_VTK_COMMON_HPP_
#define GRIDFORMAT_VTK_COMMON_HPP_

#include <type_traits>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/extended_range.hpp>
#include <gridformat/common/range_field.hpp>
#include <gridformat/common/ranges.hpp>


#include <gridformat/encoding/ascii.hpp>
#include <gridformat/encoding/base64.hpp>
#include <gridformat/encoding/raw.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/_detail.hpp>
#include <gridformat/grid/grid.hpp>

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
        case (CellType::tetrahedron): return 10;
        case (CellType::hexahedron): return 12;
    }

    throw NotImplemented("VTK cell type for the given cell type");
}

template<typename ctype, typename Grid> requires(GridDetail::exposes_point_range<Grid>)
auto make_coordinates_field(const Grid& grid) {
    return RangeField{
        points(grid)
            | std::views::all
            | std::views::transform([&] (const auto& point) {
                return make_extended<3>(coordinates(grid, point));
            }),
        Precision<ctype>{}
    };
}

template<typename HeaderType = std::size_t, Concepts::UnstructuredGrid Grid>
auto make_connectivity_field(const Grid& grid) {
    std::vector<HeaderType> connectivity;
    connectivity.reserve(number_of_cells(grid)*8);
    for (const auto& c : cells(grid))
        for (const auto& p : corners(grid, c))
            connectivity.push_back(id(grid, p));
    connectivity.shrink_to_fit();
    return RangeField{std::move(connectivity)};
    // Problem: with join_view we end up with input_iterator but
    //          we currently require forward_iterator. With c++23
    //          std::views::cache_latest this could be fixed and
    //          we may avoid having to create the connectiviy vector.
    // return RangeField{
    //     cells(grid)
    //         | std::views::all
    //         | std::views::transform([&] (const auto& cell) {
    //             return corners(grid, cell)
    //                 | std::views::all
    //                 | std::views::transform([&] (const auto& point) {
    //                     return id(grid, point);
    //                 })
    //         })
    //         | std::views::join
    // };
}

template<typename HeaderType = std::size_t, Concepts::UnstructuredGrid Grid>
auto make_offsets_field(const Grid& grid) {
    std::vector<HeaderType> offsets;
    offsets.reserve(number_of_cells(grid));
    for (const auto& c : cells(grid))
        offsets.push_back(
            offsets.empty() ? Ranges::size(corners(grid, c))
                            : Ranges::size(corners(grid, c)) + offsets.back()
        );
    return RangeField{std::move(offsets)};
    // Problem: with the "mutable" keyword, this does not compile.
    //          also, this would break multiple passes of the range...
    // return RangeField{
    //     cells(grid)
    //         | std::views::all
    //         | std::views::transform([&] (const auto& cell) {
    //             return corners(grid, cell)
    //                 | std::views::all
    //                 | std::views::transform([&] (const auto& point) {
    //                     return id(grid, point);
    //                 })
    //         })
    //         | std::views::join
    // };
}

template<Concepts::UnstructuredGrid Grid>
auto make_types_field(const Grid& grid) {
    return RangeField{
        cells(grid)
            | std::views::all
            | std::views::transform([&] (const auto& cell) {
                return VTK::cell_type_number(type(grid, cell));
            })
    };
}

//! \} group VTK

}  // namespace GridFormat::VTK

#endif  // GRIDFORMAT_VTK_COMMON_HPP_