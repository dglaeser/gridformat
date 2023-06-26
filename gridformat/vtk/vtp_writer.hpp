// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTPWriter
 */
#ifndef GRIDFORMAT_VTK_VTP_WRITER_HPP_
#define GRIDFORMAT_VTK_VTP_WRITER_HPP_

#include <ranges>
#include <ostream>
#include <iostream>
#include <algorithm>
#include <functional>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/filtered_range.hpp>
#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/logging.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace Detail {

    template<typename Grid, std::size_t size>
    struct CellTypesPredicate {
        std::reference_wrapper<const Grid> grid;
        std::array<CellType, size> cell_types;

        bool operator()(const Cell<Grid>& cell) const {
            return std::ranges::any_of(cell_types, [&] (const CellType& _ct) {
                return _ct == type(grid.get(), cell);
            });
        }
    };

    template<typename G, std::size_t s>
    CellTypesPredicate(G&&, std::array<CellType, s>&&) -> CellTypesPredicate<std::remove_cvref_t<G>, s>;
    template<typename G, std::size_t s>
    CellTypesPredicate(G&&, const std::array<CellType, s>&) -> CellTypesPredicate<std::remove_cvref_t<G>, s>;

}  // namespace Detail
#endif  // DOXYGEN

/*!
 * \ingroup VTK
 * \brief Writer for .vtu file format
 */
template<Concepts::UnstructuredGrid Grid>
class VTPWriter : public VTK::XMLWriterBase<Grid, VTPWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTPWriter<Grid>>;

    static constexpr std::array zero_d_types{CellType::vertex};
    static constexpr std::array one_d_types{CellType::segment};
    static constexpr std::array two_d_types{
        CellType::quadrilateral,
        CellType::pixel,
        CellType::polygon,
        CellType::triangle
    };

 public:
    explicit VTPWriter(const Grid& grid,
                       VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".vtp", false, std::move(xml_opts))
    {}

 private:
    VTPWriter _with(VTK::XMLOptions xml_opts) const override {
        return VTPWriter{this->grid(), std::move(xml_opts)};
    }

    void _write(std::ostream& s) const override {
        auto verts_range = _get_cell_range(Detail::CellTypesPredicate{this->grid(), zero_d_types});
        auto lines_range = _get_cell_range(Detail::CellTypesPredicate{this->grid(), one_d_types});
        auto polys_range = _get_cell_range(Detail::CellTypesPredicate{this->grid(), two_d_types});
        auto unsupported_range = _get_cell_range(
            [p=Detail::CellTypesPredicate{
                this->grid(), Ranges::merged(Ranges::merged(zero_d_types, one_d_types), two_d_types)
            }] (const Cell<Grid>& cell) {
                return !p(cell);
            }
        );

        if (Ranges::size(unsupported_range) > 0)
            std::cout << as_warning("Grid contains cell types not supported by .vtp; These will be ignored.") << std::endl;

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
            vtk_point_fields.set(name, VTK::make_vtk_field(this->_get_point_field_ptr(name)));
            this->_set_data_array(context, "Piece.PointData", name, vtk_point_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_cell_field_ptr(name)));
            this->_set_data_array(context, "Piece.CellData", name, vtk_cell_fields.get(name));
        });

        // set default active arrays (scalars, vectors, tensors)
        for (std::size_t i = 0; i <= 2; ++i)
        {
            for (const auto& [n, _] : cell_fields_of_rank(i, *this) | std::views::take(1))
                this->_set_attribute(context, "Piece.CellData", VTK::active_array_attribute[i], n);
            for (const auto& [n, _] : point_fields_of_rank(i, *this) | std::views::take(1))
                this->_set_attribute(context, "Piece.PointData", VTK::active_array_attribute[i], n);
        }

        const FieldPtr coords_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_coordinates_field<T>(this->grid(), false);
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
