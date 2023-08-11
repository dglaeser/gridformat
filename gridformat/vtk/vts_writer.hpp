// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTSWriter
 */
#ifndef GRIDFORMAT_VTK_VTS_WRITER_HPP_
#define GRIDFORMAT_VTK_VTS_WRITER_HPP_

#include <array>
#include <ranges>
#include <ostream>
#include <utility>
#include <string>
#include <optional>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/lvalue_reference.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vts file format
 */
template<Concepts::StructuredGrid Grid>
class VTSWriter : public VTK::XMLWriterBase<Grid, VTSWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTSWriter<Grid>>;
    using CType = CoordinateType<Grid>;
    static constexpr std::size_t dim = dimension<Grid>;
    static_assert(dim <= 3);

 public:
    struct Domain {
        std::array<std::size_t, dim> whole_extent;
    };

    using Offset = std::array<std::size_t, dim>;

    explicit VTSWriter(LValueReferenceOf<const Grid> grid,
                       VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".vts", true, std::move(xml_opts))
    {}

    VTSWriter as_piece_for(Domain domain) const {
        auto result = this->with(this->_xml_opts);
        result._domain = std::move(domain);
        result._offset = _offset;
        return result;
    }

    VTSWriter with_offset(Offset offset) const {
        auto result = this->with(this->_xml_opts);
        result._offset = std::move(offset);
        result._domain = _domain;
        return result;
    }

 private:
    VTSWriter _with(VTK::XMLOptions xml_opts) const override {
        return VTSWriter{this->grid(), std::move(xml_opts)};
    }

    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("StructuredGrid");
        _set_attributes(context);

        FieldStorage vtk_point_fields;
        FieldStorage vtk_cell_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_point_field_ptr(name)));
            this->_set_data_array(context, "Piece/PointData", name, vtk_cell_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_cell_field_ptr(name)));
            this->_set_data_array(context, "Piece/CellData", name, vtk_cell_fields.get(name));
        });

        const FieldPtr coords_field = std::visit([&] <typename T> (const Precision<T>&) {
            return VTK::make_coordinates_field<T>(this->grid(), true);
        }, this->_xml_settings.coordinate_precision);
        this->_set_data_array(context, "Piece/Points", "Coordinates", *coords_field);

        this->_write_xml(std::move(context), s);
    }

    void _set_attributes(typename ParentType::WriteContext& context) const {
        _set_domain_attributes(context);
        _set_extent_attributes(context);
    }

    void _set_domain_attributes(typename ParentType::WriteContext& context) const {
        using VTK::CommonDetail::extents_string;
        if (_domain)
            this->_set_attribute(context, "", "WholeExtent", extents_string(_domain->whole_extent));
        else
            this->_set_attribute(context, "", "WholeExtent", extents_string(this->grid()));
    }

    void _set_extent_attributes(typename ParentType::WriteContext& context) const {
        using VTK::CommonDetail::extents_string;
        if (int i = 0; _offset) {
            auto begin = (*_offset);
            auto end = GridFormat::extents(this->grid());
            std::ranges::for_each(end, [&] (std::integral auto& ex) { ex += begin[i++]; });
            this->_set_attribute(context, "Piece", "Extent", extents_string(begin, end));
        } else {
            this->_set_attribute(context, "Piece", "Extent", extents_string(this->grid()));
        }
    }

    std::optional<Domain> _domain;
    std::optional<Offset> _offset;
};

template<typename G>
VTSWriter(G&&, VTK::XMLOptions = {}) -> VTSWriter<std::remove_cvref_t<G>>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTS_WRITER_HPP_
