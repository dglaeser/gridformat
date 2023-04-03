// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
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
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

#ifndef DOXYGEN
namespace VTIDetail {

    template<Concepts::StaticallySizedRange R>
    std::string number_string_3d(const R& r) {
        if constexpr (static_size<std::decay_t<R>> == 3)
            return as_string(r);
        if constexpr (static_size<std::decay_t<R>> == 2)
            return as_string(r) + " 0";
        if constexpr (static_size<std::decay_t<R>> == 1)
            return as_string(r) + " 0 0";
    }

    template<Concepts::StaticallySizedRange R>
    std::string extents_string(const R& r) {
        int i = 0;
        std::string result;
        std::ranges::for_each(r, [&] (const auto& entry) {
            result += (i > 0 ? " 0 " : "0 ") + as_string(entry);
            ++i;
        });
        for (i = static_size<R>; i < 3; ++i)
            result += " 0 0";
        return result;
    }
    template<Concepts::StaticallySizedRange R1,
             Concepts::StaticallySizedRange R2>
    std::string extents_string(const R1& r1, const R2& r2) {
        static_assert(static_size<R1> == static_size<R2>);
        int i = 0;
        std::string result;
        auto it1 = std::ranges::begin(r1);
        auto it2 = std::ranges::begin(r2);
        for (; it1 != std::ranges::end(r1); ++it1, ++it2, ++i)
            result += (i > 0 ? " " : "")
                        + as_string(*it1) + " "
                        + as_string(*it2);
        for (i = static_size<R1>; i < 3; ++i)
            result += " 0 0";
        return result;
    }

    template<Concepts::StructuredGrid Grid>
    std::string extents_string(const Grid& grid) {
        return extents_string(extents(grid));
    }

}  // namespace VTIDetail
#endif  // DOXYGEN


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

    explicit VTIWriter(const Grid& grid, VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".vti", std::move(xml_opts))
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
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_shared_point_field(name)));
            this->_set_data_array(context, "Piece.PointData", name, vtk_cell_fields.get(name));
        });
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            vtk_cell_fields.set(name, VTK::make_vtk_field(this->_get_shared_cell_field(name)));
            this->_set_data_array(context, "Piece.CellData", name, vtk_cell_fields.get(name));
        });

        this->_write_xml(std::move(context), s);
    }

    void _set_attributes(typename ParentType::WriteContext& context) const {
        _set_domain_attributes(context);
        _set_extent_attributes(context);
        this->_set_attribute(context, "", "Spacing", VTIDetail::number_string_3d(spacing(this->grid())));
    }

    void _set_domain_attributes(typename ParentType::WriteContext& context) const {
        if (_domain) {
            this->_set_attribute(context, "", "WholeExtent", VTIDetail::extents_string(_domain->whole_extent));
            this->_set_attribute(context, "", "Origin", VTIDetail::number_string_3d(_domain->origin));
        } else {
            this->_set_attribute(context, "", "WholeExtent", VTIDetail::extents_string(this->grid()));
            this->_set_attribute(context, "", "Origin", VTIDetail::number_string_3d(origin(this->grid())));
        }
    }

    void _set_extent_attributes(typename ParentType::WriteContext& context) const {
        if (int i = 0; _offset) {
            auto begin = (*_offset);
            auto end = GridFormat::extents(this->grid());
            std::ranges::for_each(end, [&] (std::integral auto& ex) { ex += begin[i++]; });
            this->_set_attribute(context, "Piece", "Extent", VTIDetail::extents_string(begin, end));
        } else {
            this->_set_attribute(context, "Piece", "Extent", VTIDetail::extents_string(this->grid()));
        }
    }

    std::optional<Domain> _domain;
    std::optional<Offset> _offset;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTI_WRITER_HPP_
