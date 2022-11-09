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
#include <gridformat/vtk/common.hpp>

namespace GridFormat {

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

        auto context = this->_get_write_context("UnstructuredGrid");
        this->_set_attribute(context, "Piece", "NumberOfPoints", num_points);
        this->_set_attribute(context, "Piece", "NumberOfCells", num_cells);
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& n) {
            this->_set_data_array(context, "Piece.PointData", n, this->_get_point_field(n));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& n) {
            this->_set_data_array(context, "Piece.CellData", n, this->_get_cell_field(n));
        });
        invoke_with_precision(this->_coordinate_precision, [&] <typename T> (const Precision<T>&) {
            this->_set_data_array(context, "Piece.Points", "Coordinates", VTK::make_points_field<T>(this->_grid));
        });
        invoke_with_precision(this->_header_precision, [&] <typename T> (const Precision<T>&) {
            this->_set_data_array(context, "Piece.Cells", "connectivity", VTK::make_connectivity_field<T>(this->_grid));
        });
        invoke_with_precision(this->_header_precision, [&] <typename T> (const Precision<T>&) {
            this->_set_data_array(context, "Piece.Cells", "offsets", VTK::make_offsets_field<T>(this->_grid));
        });
        this->_set_data_array(context, "Piece.Cells", "types", VTK::make_types_field(this->_grid));
        this->_write_xml(context, s);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_