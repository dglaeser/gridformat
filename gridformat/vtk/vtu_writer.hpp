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

#include <gridformat/common/field.hpp>
#include <gridformat/common/flat_field.hpp>
#include <gridformat/common/transformed_fields.hpp>

#include <gridformat/grid/grid.hpp>
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

    template<typename T>
    std::unique_ptr<std::decay_t<T>> _make_unique_from_instance(T&& t) const {
        return std::make_unique<std::decay_t<T>>(std::move(t));
    }

    void _write(std::ostream& s) const override {
        const auto num_points = number_of_points(this->_get_grid());
        const auto num_cells = number_of_cells(this->_get_grid());
        auto context = this->_get_write_context("UnstructuredGrid");

        std::list<std::unique_ptr<Field>> point_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& n) {
            const Field& field = this->_get_point_field(n);
            if (field.layout().dimension() > 1)
                point_fields.emplace_back(this->_make_unique_from_instance(
                    TransformedField{
                        TransformedField{field, FieldTransformation::extended(3)},
                        FieldTransformation::flattened
                    }
                ));
            else
                point_fields.emplace_back(this->_make_unique_from_instance(
                    TransformedField{field, FieldTransformation::identity}
                ));
            this->_set_data_array(context, "Piece.PointData", n, *point_fields.back());
        });

        std::list<std::unique_ptr<Field>> cell_fields;
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& n) {
            const Field& field = this->_get_cell_field(n);
            if (field.layout().dimension() > 1)
                cell_fields.emplace_back(this->_make_unique_from_instance(
                    TransformedField{
                        TransformedField{field, FieldTransformation::extended(3)},
                        FieldTransformation::flattened
                    }
                ));
            else
                cell_fields.emplace_back(this->_make_unique_from_instance(
                    TransformedField{field, FieldTransformation::identity}
                ));
            this->_set_data_array(context, "Piece.CellData", n, *cell_fields.back());
        });

        this->_set_attribute(context, "Piece", "NumberOfPoints", num_points);
        this->_set_attribute(context, "Piece", "NumberOfCells", num_cells);

        const auto coords_range_field = VTK::make_coordinates_range_field<CoordinateType>(this->_get_grid());
        const auto coords_field = make_3d(coords_range_field);
        const auto connectivity_field = VTK::make_connectivity_range_field<HeaderType>(this->_get_grid());
        const auto offsets_field = VTK::make_offsets_field<HeaderType>(this->_get_grid());
        const auto types_field = VTK::make_types_field(this->_get_grid());
        this->_set_data_array(context, "Piece.Points", "Coordinates", coords_field);
        this->_set_data_array(context, "Piece.Cells", "connectivity", connectivity_field);
        this->_set_data_array(context, "Piece.Cells", "offsets", offsets_field);
        this->_set_data_array(context, "Piece.Cells", "types", types_field);
        this->_write_xml(std::move(context), s);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_WRITER_HPP_
