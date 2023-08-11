// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTSReader
 */
#ifndef GRIDFORMAT_VTK_VTS_READER_HPP_
#define GRIDFORMAT_VTK_VTS_READER_HPP_

#include <string>
#include <optional>
#include <array>

#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .vts file format
 */
class VTSReader : public GridReader {
 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "StructuredGrid");
        _extents = Ranges::array_from_string<std::size_t, 6>(helper.get("StructuredGrid/Piece").get_attribute("Extent"));
        VTK::XMLDetail::copy_field_names_from(helper.get("StructuredGrid"), fields);
        _helper.emplace(std::move(helper));
    }

    void _close() override {
        _helper.reset();
        _extents.reset();
    }

    std::string _name() const override {
        return "VTSReader";
    }

    std::size_t _number_of_cells() const override {
        return VTK::CommonDetail::number_of_entities(_extents.value());
    }

    std::size_t _number_of_points() const override {
        auto pextents = _extents.value();
        pextents[1] += 1;
        pextents[3] += 1;
        pextents[5] += 1;
        return VTK::CommonDetail::number_of_entities(pextents);
    }

    std::size_t _number_of_pieces() const override {
        return 1;
    }

    typename GridReader::PieceLocation _location() const override {
        const auto& ex = _extents.value();
        typename GridReader::PieceLocation result;
        result.lower_left = {ex[0], ex[2], ex[4]};
        result.upper_right = {ex[1], ex[3], ex[5]};
        return result;
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _points() const override {
        return _helper.value().make_points_field("StructuredGrid/Piece/Points", _number_of_points());
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        VTK::CommonDetail::visit_structured_cells(visitor, _extents.value(), false);
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "StructuredGrid/Piece/CellData", _number_of_cells());
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "StructuredGrid/Piece/PointData", _number_of_points());
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "StructuredGrid/FieldData");
    }

    std::optional<VTK::XMLReaderHelper> _helper;
    std::optional<std::array<std::size_t, 6>> _extents;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTS_READER_HPP_
