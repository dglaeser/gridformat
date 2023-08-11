// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTPReader
 */
#ifndef GRIDFORMAT_VTK_VTP_READER_HPP_
#define GRIDFORMAT_VTK_VTP_READER_HPP_

#include <string>
#include <optional>
#include <iterator>
#include <utility>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .vtp file format
 */
class VTPReader : public GridReader {
 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "PolyData");

        _num_points = from_string<std::size_t>(helper.get("PolyData/Piece").get_attribute("NumberOfPoints"));
        _num_verts = helper.get("PolyData/Piece").get_attribute_or(std::size_t{0}, "NumberOfVerts");
        _num_lines = helper.get("PolyData/Piece").get_attribute_or(std::size_t{0}, "NumberOfLines");
        _num_strips = helper.get("PolyData/Piece").get_attribute_or(std::size_t{0}, "NumberOfStrips");
        _num_polys = helper.get("PolyData/Piece").get_attribute_or(std::size_t{0}, "NumberOfPolys");

        if (_num_strips > 0)
            throw NotImplemented("Triangle strips are not (yet) supported");

        VTK::XMLDetail::copy_field_names_from(helper.get("PolyData"), fields);
        _helper.emplace(std::move(helper));
    }

    void _close() override {
        _helper.reset();
        _num_points = 0;
        _num_verts = 0;
        _num_lines = 0;
        _num_strips = 0;
        _num_polys = 0;
    }

    std::string _name() const override {
        return "VTPReader";
    }

    std::size_t _number_of_cells() const override {
        return _num_verts + _num_lines + _num_strips + _num_polys;
    }

    std::size_t _number_of_points() const override {
        return _num_points;
    }

    std::size_t _number_of_pieces() const override {
        return 1;
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _points() const override {
        return _helper.value().make_points_field("PolyData/Piece/Points", _number_of_points());
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        if (_num_verts > 0)
            _visit_cells("Verts", CellType::vertex, _num_verts, visitor);
        if (_num_lines > 0)
            _visit_cells("Lines", CellType::segment, _num_lines, visitor);
        if (_num_polys > 0)
            _visit_cells("Polys", CellType::polygon, _num_polys, visitor);
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "PolyData/Piece/CellData", _number_of_cells());
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "PolyData/Piece/PointData", _number_of_points());
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "PolyData/FieldData");
    }

    void _visit_cells(std::string_view type_name,
                      const CellType& cell_type,
                      const std::size_t expected_size,
                      const typename GridReader::CellVisitor& visitor) const {
        const std::string path = "PolyData/Piece/" + std::string{type_name};
        const auto offsets = _helper.value().make_data_array_field(
            "offsets", path, expected_size
        )->template export_to<std::vector<std::size_t>>();
        const auto connectivity = _helper.value().make_data_array_field(
            "connectivity", path
        )->template export_to<std::vector<std::size_t>>();

        std::vector<std::size_t> corners;
        for (std::size_t i = 0; i < expected_size; ++i) {
            corners.clear();
            const std::size_t offset_begin = (i == 0 ? 0 : offsets.at(i-1));
            const std::size_t offset_end = offsets.at(i);
            if (connectivity.size() < offset_end)
                throw SizeError("Connectivity array read from the file is too small");
            if (offset_end < offset_begin)
                throw ValueError("Invalid offset array");
            std::copy_n(connectivity.begin() + offset_begin, offset_end - offset_begin, std::back_inserter(corners));

            const CellType ct = cell_type == CellType::polygon ? (
                    corners.size() == 3 ? CellType::triangle
                                        : (corners.size() == 4 ? CellType::quadrilateral : cell_type)
                ) : cell_type;
            visitor(ct, corners);
        }
    }

    std::optional<VTK::XMLReaderHelper> _helper;
    std::size_t _num_points;
    std::size_t _num_verts;
    std::size_t _num_lines;
    std::size_t _num_strips;
    std::size_t _num_polys;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTP_READER_HPP_
