// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief TODO: Doc me
 */
#ifndef GRIDFORMAT_VTK_VTU_WRITER_HPP_
#define GRIDFORMAT_VTK_VTU_WRITER_HPP_

#include <ranges>
#include <ostream>

#include <gridformat/common/extended_range.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/grid/grid.hpp>

#include <gridformat/vtk/xml_writer_base.hpp>
#include <gridformat/vtk/options.hpp>


#include <gridformat/common/range_field.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/vtk/common.hpp>

namespace GridFormat {

template<typename Grid>
auto make_points_field(const Grid& grid) {
    return RangeField{
        points(grid)
            | std::views::all
            | std::views::transform([&] (const auto& point) {
                return make_extended<3>(coordinates(grid, point));
            })
    };
}

template<typename Grid>
auto make_connectivity_field(const Grid& grid) {
    std::vector<std::size_t> connectivity;
    connectivity.reserve(number_of_cells(grid)*8);
    for (const auto& c : cells(grid))
        for (const auto& p : corners(grid, c))
            connectivity.push_back(id(grid, p));
    connectivity.shrink_to_fit();
    return RangeField{std::move(connectivity)};
    // Problem: with join_view we end up with input_iterator
    //          but we currently require forward_iterator...
    //          Do we really need forward ranges?
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

template<typename Grid>
auto make_offsets_field(const Grid& grid) {
    std::vector<std::size_t> offsets;
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

template<typename Grid>
auto make_types_field(const Grid& grid) {
    return RangeField{
        cells(grid)
            | std::views::all
            | std::views::transform([&] (const auto& cell) {
                return VTK::cell_type_number(type(grid, cell));
            })
    };
}

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<Concepts::UnstructuredGrid Grid,
         typename Format = VTK::DataFormat::Inlined,
         typename Encoding = VTK::Encoding::Ascii>
class VTUWriter : public VTK::XMLWriterBase<Grid, Format, Encoding> {
    using ParentType = VTK::XMLWriterBase<Grid>;

 public:
    explicit VTUWriter(const Grid& grid,
                       const Format& format = {},
                       const Encoding& encoding = {})
    : ParentType(grid, ".vtu", format, encoding)
    {}

 private:
    void _write([[maybe_unused]] std::ostream& s) const override {
        const auto num_points = number_of_points(this->_grid);
        const auto num_cells = number_of_cells(this->_grid);

        const auto point_coords_field = make_points_field(this->_grid);
        const auto connectivity_field = make_connectivity_field(this->_grid);
        const auto offsets_field = make_offsets_field(this->_grid);
        const auto types_field = make_types_field(this->_grid);

        auto context = this->_get_write_context("UnstructuredGrid");
        this->_set_attribute(context, "Piece", "NumberOfPoints", num_points);
        this->_set_attribute(context, "Piece", "NumberOfCells", num_cells);
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& n) {
            this->_set_data_array(context, "Piece.PointData", n, this->_get_point_field(n));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& n) {
            this->_set_data_array(context, "Piece.CellData", n, this->_get_cell_field(n));
        });
        this->_set_data_array(context, "Piece.Points", "Coordinates", point_coords_field);
        this->_set_data_array(context, "Piece.Cells", "connectivity", connectivity_field);
        this->_set_data_array(context, "Piece.Cells", "offsets", offsets_field);
        this->_set_data_array(context, "Piece.Cells", "types", types_field);
        this->_write_xml(context, s);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_