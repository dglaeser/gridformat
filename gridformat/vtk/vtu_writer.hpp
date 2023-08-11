// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
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
#include <gridformat/common/lvalue_reference.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vtu file format
 */
template<Concepts::UnstructuredGrid Grid>
class VTUWriter : public VTK::XMLWriterBase<Grid, VTUWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTUWriter<Grid>>;

 public:
    explicit VTUWriter(LValueReferenceOf<const Grid> grid,
                       VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".vtu", false, std::move(xml_opts))
    {}

 private:
    VTUWriter _with(VTK::XMLOptions xml_opts) const override {
        return VTUWriter{this->grid(), std::move(xml_opts)};
    }

    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("UnstructuredGrid");
        this->_set_attribute(context, "Piece", "NumberOfPoints", number_of_points(this->grid()));
        this->_set_attribute(context, "Piece", "NumberOfCells", number_of_cells(this->grid()));

        FieldStorage vtk_point_fields;
        FieldStorage vtk_cell_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            vtk_point_fields.set(name, VTK::make_vtk_field(this->_get_point_field_ptr(name)));
            this->_set_data_array(context, "Piece/PointData", name, vtk_point_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_cell_field_ptr(name)));
            this->_set_data_array(context, "Piece/CellData", name, vtk_cell_fields.get(name));
        });

        const auto point_id_map = make_point_id_map(this->grid());
        const FieldPtr coords_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_coordinates_field<T>(this->grid(), false);
        }, this->_xml_settings.coordinate_precision);
        const FieldPtr connectivity_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_connectivity_field<T>(this->grid(), point_id_map);
        }, this->_xml_settings.header_precision);
        const FieldPtr offsets_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_offsets_field<T>(this->grid());
        }, this->_xml_settings.header_precision);
        const FieldPtr types_field = VTK::make_cell_types_field(this->grid());
        this->_set_data_array(context, "Piece/Points", "Coordinates", *coords_field);
        this->_set_data_array(context, "Piece/Cells", "connectivity", *connectivity_field);
        this->_set_data_array(context, "Piece/Cells", "offsets", *offsets_field);
        this->_set_data_array(context, "Piece/Cells", "types", *types_field);
        this->_write_xml(std::move(context), s);
    }
};

template<typename G>
VTUWriter(G&&, VTK::XMLOptions = {}) -> VTUWriter<std::remove_cvref_t<G>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_
