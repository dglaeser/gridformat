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
template<Concepts::UnstructuredGrid Grid>
class VTPWriter : public VTK::XMLWriterBase<Grid, VTPWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTPWriter<Grid>>;

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
                       VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".vtp", std::move(xml_opts))
    {}

    VTPWriter with(VTK::XMLOptions xml_opts) const {
        return VTPWriter{this->grid(), std::move(xml_opts)};
    }

 private:
    void _write(std::ostream& s) const override {
        auto verts_range = _get_cell_range(
            CellTypesPredicate<1>{this->grid(), {CellType::vertex}}
        );
        auto lines_range = _get_cell_range(
            CellTypesPredicate<1>{this->grid(), {CellType::segment}}
        );
        auto polys_range = _get_cell_range(
            CellTypesPredicate<4>{this->grid(), {CellType::quadrilateral, CellType::rectangle, CellType::polygon, CellType::triangle}}
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

        const FieldPtr coords_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_coordinates_field<T>(this->grid());
        }, this->_xml_settings.coordinate_precision);
        this->_set_data_array(context, "Piece.Points", "Coordinates", *coords_field);

        const auto point_id_map = make_point_id_map(this->grid());
        const auto verts_connectivity_field = _make_connectivity_field(verts_range, point_id_map);
        const auto verts_offsets_field = _make_offsets_field(verts_range);
        this->_set_data_array(context, "Piece.Verts", "connectivity", *verts_connectivity_field);
        this->_set_data_array(context, "Piece.Verts", "offsets", *verts_offsets_field);

        const auto lines_connectivity_field = _make_connectivity_field(lines_range, point_id_map);
        const auto lines_offsets_field = _make_offsets_field(lines_range);
        this->_set_data_array(context, "Piece.Lines", "connectivity", *lines_connectivity_field);
        this->_set_data_array(context, "Piece.Lines", "offsets", *lines_offsets_field);

        const auto polys_connectivity_field = _make_connectivity_field(polys_range, point_id_map);
        const auto polys_offsets_field = _make_offsets_field(polys_range);
        this->_set_data_array(context, "Piece.Polys", "connectivity", *polys_connectivity_field);
        this->_set_data_array(context, "Piece.Polys", "offsets", *polys_offsets_field);

        this->_write_xml(std::move(context), s);
    }

    template<typename Predicate>
    std::ranges::forward_range auto _get_cell_range(Predicate&& pred) const {
        return Ranges::filter_by(std::forward<Predicate>(pred), cells(this->grid()));
    }

    template<typename CellsRange, typename PointMap>
    FieldPtr _make_connectivity_field(CellsRange&& cells, const PointMap& point_id_map) const {
        return std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_connectivity_field<T>(this->grid(), cells, point_id_map);
        }, this->_xml_settings.header_precision);
    }

    template<typename CellsRange>
    FieldPtr _make_offsets_field(CellsRange&& cells) const {
        return std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_offsets_field<T>(this->grid(), cells);
        }, this->_xml_settings.header_precision);
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTP_WRITER_HPP_
