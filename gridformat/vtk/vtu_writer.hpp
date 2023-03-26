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
#include <gridformat/common/field_storage.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vtu file format
 */
template<Concepts::UnstructuredGrid Grid>
class VTUWriter : public VTK::XMLWriterBase<Grid> {
    using ParentType = VTK::XMLWriterBase<Grid>;

 public:
    explicit VTUWriter(const Grid& grid,
                       VTK::XMLOptions xml_opts = {},
                       VTK::PrecisionOptions prec_opts = {})
    : ParentType(grid, ".vtu", std::move(xml_opts), std::move(prec_opts))
    {}

 private:
    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("UnstructuredGrid");
        this->_set_attribute(context, "Piece", "NumberOfPoints", number_of_points(this->grid()));
        this->_set_attribute(context, "Piece", "NumberOfCells", number_of_cells(this->grid()));

        FieldStorage vtk_point_fields;
        FieldStorage vtk_cell_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            vtk_point_fields.set(name, VTK::make_vtk_field(this->_get_shared_point_field(name)));
            this->_set_data_array(context, "Piece.PointData", name, vtk_point_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_shared_cell_field(name)));
            this->_set_data_array(context, "Piece.CellData", name, vtk_cell_fields.get(name));
        });

        const auto point_id_map = make_point_id_map(this->grid());
        const FieldPtr coords_field = this->_coord_precision.visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_coordinates_field<T>(this->grid());
        });
        const FieldPtr connectivity_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_connectivity_field<T>(this->grid(), point_id_map);
        }, this->_header_precision);
        const FieldPtr offsets_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_offsets_field<T>(this->grid());
        }, this->_header_precision);
        const FieldPtr types_field = VTK::make_cell_types_field(this->grid());
        this->_set_data_array(context, "Piece.Points", "Coordinates", *coords_field);
        this->_set_data_array(context, "Piece.Cells", "connectivity", *connectivity_field);
        this->_set_data_array(context, "Piece.Cells", "offsets", *offsets_field);
        this->_set_data_array(context, "Piece.Cells", "types", *types_field);
        this->_write_xml(std::move(context), s);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_
