// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTIWriter
 */
#ifndef GRIDFORMAT_VTK_VTI_WRITER_HPP_
#define GRIDFORMAT_VTK_VTI_WRITER_HPP_

#include <ranges>
#include <ostream>
#include <utility>
#include <string>

#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vti file format
 */
template<Concepts::ImageGrid Grid>
class VTIWriter : public VTK::XMLWriterBase<Grid, VTIWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTIWriter<Grid>>;

 public:
    explicit VTIWriter(const Grid& grid, VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".vti", std::move(xml_opts))
    {}

    VTIWriter with(VTK::XMLOptions xml_opts) const {
        return VTIWriter{this->grid(), std::move(xml_opts)};
    }

 private:
    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("ImageData");
        this->_set_attribute(context, "", "WholeExtent", _make_extents());
        this->_set_attribute(context, "", "Origin", _3d_number_string(origin(this->grid())));
        this->_set_attribute(context, "", "Spacing", _3d_number_string(spacing(this->grid())));
        this->_set_attribute(context, "Piece", "Extent", _make_extents());

        FieldStorage vtk_point_fields;
        FieldStorage vtk_cell_fields;
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_shared_point_field(name)));
            this->_set_data_array(context, "Piece.PointData", name, vtk_cell_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_shared_cell_field(name)));
            this->_set_data_array(context, "Piece.CellData", name, vtk_cell_fields.get(name));
        });

        this->_write_xml(std::move(context), s);
    }

    template<Concepts::StaticallySizedRange R>
    std::string _3d_number_string(R&& r) const {
        static_assert(static_size<std::decay_t<R>> == dimension<Grid>);
        if constexpr (dimension<Grid> == 3)
            return as_string(std::forward<R>(r));
        if constexpr (dimension<Grid> == 2)
            return as_string(std::forward<R>(r)) + " 0";
        if constexpr (dimension<Grid> == 1)
            return as_string(std::forward<R>(r) + " 0 0");
    }

    std::string _make_extents() const {
        const auto max = extents(this->grid());
        std::string result;
        for (std::size_t i = 0; i < dimension<Grid>; ++i)
            result += (i > 0 ? " 0 " : "0 ") + std::to_string(max[i]);
        for (std::size_t i = dimension<Grid>; i < 3; ++i)
            result += " 0 0";
        return result;
    }
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTI_WRITER_HPP_
