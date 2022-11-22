// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTUWriter
 */
#ifndef GRIDFORMAT_VTK_VTU_WRITER_HPP_
#define GRIDFORMAT_VTK_VTU_WRITER_HPP_

#include <ranges>
#include <ostream>
#include <string>

#include <gridformat/common/field.hpp>
#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/field_cache.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vtu file format
 */
template<Concepts::UnstructuredGrid Grid,
         typename XMLOpts = VTK::XMLOptions<>,
         typename PrecOpts = VTK::PrecisionOptions<>>
class VTUWriter : public VTK::XMLWriterBase<Grid, XMLOpts, PrecOpts> {
    using ParentType = VTK::XMLWriterBase<Grid, XMLOpts, PrecOpts>;

 public:
    explicit VTUWriter(const Grid& grid,
                       XMLOpts xml_opts = {},
                       PrecOpts prec_opts = {})
    : ParentType(grid, ".vtu", std::move(xml_opts), std::move(prec_opts))
    {}

 private:
    using typename ParentType::CoordinateType;
    using typename ParentType::HeaderType;

    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("UnstructuredGrid");
        this->_set_attribute(context, "Piece", "NumberOfPoints", number_of_points(this->_get_grid()));
        this->_set_attribute(context, "Piece", "NumberOfCells", number_of_cells(this->_get_grid()));

        VTK::FieldCache point_fields;
        VTK::FieldCache cell_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            const Field& field = point_fields.insert(this->_get_point_field(name));
            this->_set_data_array(context, "Piece.PointData", name, field);
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            const Field& field = point_fields.insert(this->_get_cell_field(name));
            this->_set_data_array(context, "Piece.CellData", name, field);
        });

        const auto coords_field = VTK::make_coordinates_field<CoordinateType>(this->_get_grid());
        const auto connectivity_field = VTK::make_connectivity_field<HeaderType>(this->_get_grid());
        const auto offsets_field = VTK::make_offsets_field<HeaderType>(this->_get_grid());
        const auto types_field = VTK::make_cell_types_field(this->_get_grid());
        this->_set_data_array(context, "Piece.Points", "Coordinates", coords_field);
        this->_set_data_array(context, "Piece.Cells", "connectivity", connectivity_field);
        this->_set_data_array(context, "Piece.Cells", "offsets", offsets_field);
        this->_set_data_array(context, "Piece.Cells", "types", types_field);
        this->_write_xml(std::move(context), s);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_
