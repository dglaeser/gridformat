// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTPWriter
 */
#ifndef GRIDFORMAT_VTK_VTP_WRITER_HPP_
#define GRIDFORMAT_VTK_VTP_WRITER_HPP_

#include <ranges>
#include <ostream>
#include <functional>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/filtered_range.hpp>
#include <gridformat/common/field_storage.hpp>

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
class VTPWriter : public VTK::XMLWriterBase<Grid, XMLOpts, PrecOpts> {
    using ParentType = VTK::XMLWriterBase<Grid, XMLOpts, PrecOpts>;

    template<std::size_t size>
    struct CellTypesPredicate {
        std::reference_wrapper<const Grid> grid;
        std::array<CellType, size> cell_types;

        bool operator()(const Cell<Grid>& cell) const {
            return std::ranges::any_of(cell_types, [&] (const CellType& _ct) {
                return _ct == type(grid.get(), cell);
            });
        }
    };

 public:
    explicit VTPWriter(const Grid& grid,
                       XMLOpts xml_opts = {},
                       PrecOpts prec_opts = {})
    : ParentType(grid, ".vtp", std::move(xml_opts), std::move(prec_opts))
    {}

 private:
    using typename ParentType::CoordinateType;
    using typename ParentType::HeaderType;

    void _write(std::ostream& s) const override {
        auto verts_range = _get_cell_range(
            CellTypesPredicate<1>{this->grid(), {CellType::vertex}}
        );
        auto lines_range = _get_cell_range(
            CellTypesPredicate<1>{this->grid(), {CellType::segment}}
        );
        auto polys_range = _get_cell_range(
            CellTypesPredicate<3>{this->grid(), {CellType::quadrilateral, CellType::polygon, CellType::triangle}}
        );

        const auto num_verts = Ranges::size(verts_range);
        const auto num_lines = Ranges::size(lines_range);
        const auto num_polys = Ranges::size(polys_range);

        auto context = this->_get_write_context("PolyData");
        this->_set_attribute(context, "Piece", "NumberOfPoints", number_of_points(this->grid()));
        this->_set_attribute(context, "Piece", "NumberOfVerts", num_verts);
        this->_set_attribute(context, "Piece", "NumberOfLines", num_lines);
        this->_set_attribute(context, "Piece", "NumberOfStrips", "0");
        this->_set_attribute(context, "Piece", "NumberOfPolys", num_polys);

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

        const auto coords_field = VTK::make_coordinates_field<CoordinateType>(this->grid());
        this->_set_data_array(context, "Piece.Points", "Coordinates", *coords_field);

        const auto point_id_map = make_point_id_map(this->grid());
        const auto verts_connectivity_field = VTK::make_connectivity_field<HeaderType>(this->grid(), verts_range, point_id_map);
        const auto verts_offsets_field = VTK::make_offsets_field<HeaderType>(this->grid(), verts_range);
        this->_set_data_array(context, "Piece.Verts", "connectivity", *verts_connectivity_field);
        this->_set_data_array(context, "Piece.Verts", "offsets", *verts_offsets_field);

        const auto lines_connectivity_field = VTK::make_connectivity_field<HeaderType>(this->grid(), lines_range, point_id_map);
        const auto lines_offsets_field = VTK::make_offsets_field<HeaderType>(this->grid(), lines_range);
        this->_set_data_array(context, "Piece.Lines", "connectivity", *lines_connectivity_field);
        this->_set_data_array(context, "Piece.Lines", "offsets", *lines_offsets_field);

        const auto polys_connectivity_field = VTK::make_connectivity_field<HeaderType>(this->grid(), polys_range, point_id_map);
        const auto polys_offsets_field = VTK::make_offsets_field<HeaderType>(this->grid(), polys_range);
        this->_set_data_array(context, "Piece.Polys", "connectivity", *polys_connectivity_field);
        this->_set_data_array(context, "Piece.Polys", "offsets", *polys_offsets_field);

        this->_write_xml(std::move(context), s);
    }

    template<typename Predicate>
    std::ranges::forward_range auto _get_cell_range(Predicate&& pred) const {
        return FilteredRange{cells(this->grid()), std::forward<Predicate>(pred)};
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTP_WRITER_HPP_
