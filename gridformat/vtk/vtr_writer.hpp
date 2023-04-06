// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTRWriter
 */
#ifndef GRIDFORMAT_VTK_VTR_WRITER_HPP_
#define GRIDFORMAT_VTK_VTR_WRITER_HPP_

#include <array>
#include <ranges>
#include <ostream>
#include <utility>
#include <string>
#include <optional>
#include <iterator>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_storage.hpp>

#include <gridformat/grid/grid.hpp>
#include <gridformat/grid/type_traits.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Writer for .vtr file format
 */
template<Concepts::RectilinearGrid Grid>
class VTRWriter : public VTK::XMLWriterBase<Grid, VTRWriter<Grid>> {
    using ParentType = VTK::XMLWriterBase<Grid, VTRWriter<Grid>>;
    using CType = CoordinateType<Grid>;
    static constexpr std::size_t dim = dimension<Grid>;
    static constexpr std::size_t space_dim = 3;
    static_assert(dim <= 3);

 public:
    struct Domain {
        std::array<std::size_t, dim> whole_extent;
    };

    using Offset = std::array<std::size_t, dim>;

    explicit VTRWriter(const Grid& grid, VTK::XMLOptions xml_opts = {})
    : ParentType(grid, ".vtr", std::move(xml_opts))
    {}

    VTRWriter as_piece_for(Domain domain) const {
        auto result = this->with(this->_xml_opts);
        result._domain = std::move(domain);
        result._offset = _offset;
        return result;
    }

    VTRWriter with_offset(Offset offset) const {
        auto result = this->with(this->_xml_opts);
        result._offset = std::move(offset);
        result._domain = _domain;
        return result;
    }

 private:
    VTRWriter _with(VTK::XMLOptions xml_opts) const override {
        return VTRWriter{this->grid(), std::move(xml_opts)};
    }

    void _write(std::ostream& s) const override {
        auto context = this->_get_write_context("RectilinearGrid");
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

        const auto coord_fields = _make_ordinate_fields();
        for (unsigned dir = 0; dir < space_dim; ++dir)
            this->_set_data_array(context, "Piece.Coordinates", "X_" + std::to_string(dir), *coord_fields[dir]);
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

    std::array<FieldPtr, space_dim> _make_ordinate_fields() const {
        std::array<FieldPtr, space_dim> result;
        std::visit([&] <typename T> (const Precision<T>& prec) {
            for (unsigned dir = 0; dir < space_dim; ++dir) {
                if (dir < dim) {
                    assert(std::ranges::is_sorted(ordinates(this->grid(), dir)));
                    result[dir] = make_shared(RangeField{ordinates(this->grid(), dir), prec});
                }
                else
                    result[dir] = make_shared(RangeField{std::vector<double>{}, prec});
            }
        }, this->_xml_settings.coordinate_precision);
        return result;
    }

    std::optional<Domain> _domain;
    std::optional<Offset> _offset;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTR_WRITER_HPP_
