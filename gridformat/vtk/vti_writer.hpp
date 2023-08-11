// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTIWriter
 */
#ifndef GRIDFORMAT_VTK_VTI_WRITER_HPP_
#define GRIDFORMAT_VTK_VTI_WRITER_HPP_

#include <array>
#include <ranges>
#include <ostream>
#include <utility>
#include <string>
#include <optional>

#include <gridformat/common/field_storage.hpp>
#include <gridformat/common/lvalue_reference.hpp>

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
    using CType = CoordinateType<Grid>;
    static constexpr std::size_t dim = dimension<Grid>;

 public:
    struct Domain {
        std::array<CType, dim> origin;
        std::array<std::size_t, dim> whole_extent;
    };

    using Offset = std::array<std::size_t, dim>;

    explicit VTIWriter(LValueReferenceOf<const Grid> grid,
                       VTK::XMLOptions xml_opts = {})
    : ParentType(grid.get(), ".vti", true, std::move(xml_opts))
    {}

    VTIWriter as_piece_for(Domain domain) const {
        auto result = this->with(this->_xml_opts);
        result._domain = std::move(domain);
        result._offset = _offset;
        return result;
    }

    VTIWriter with_offset(Offset offset) const {
        auto result = this->with(this->_xml_opts);
        result._offset = std::move(offset);
        result._domain = _domain;
        return result;
    }

 private:
    VTIWriter _with(VTK::XMLOptions xml_opts) const override {
        return VTIWriter{this->grid(), std::move(xml_opts)};
    }

    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("ImageData");
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

        this->_write_xml(std::move(context), s);
    }

    void _set_attributes(typename ParentType::WriteContext& context) const {
        _set_domain_attributes(context);
        _set_extent_attributes(context);
        this->_set_attribute(context, "", "Spacing", VTK::CommonDetail::number_string_3d(spacing(this->grid())));
        this->_set_attribute(context, "", "Direction", VTK::CommonDetail::direction_string(basis(this->grid())));
    }

    void _set_domain_attributes(typename ParentType::WriteContext& context) const {
        using VTK::CommonDetail::extents_string;
        using VTK::CommonDetail::number_string_3d;
        if (_domain) {
            this->_set_attribute(context, "", "WholeExtent", extents_string(_domain->whole_extent));
            this->_set_attribute(context, "", "Origin", number_string_3d(_domain->origin));
        } else {
            this->_set_attribute(context, "", "WholeExtent", extents_string(this->grid()));
            this->_set_attribute(context, "", "Origin", number_string_3d(origin(this->grid())));
        }
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
VTIWriter(G&&, VTK::XMLOptions opts = {}) -> VTIWriter<std::remove_cvref_t<G>>;

namespace Traits {

template<typename... Args>
struct WritesConnectivity<VTIWriter<Args...>> : public std::false_type {};

}  // namespace Traits
}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTI_WRITER_HPP_
