// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTUReader
 */
#ifndef GRIDFORMAT_VTK_VTU_READER_HPP_
#define GRIDFORMAT_VTK_VTU_READER_HPP_

#include <string>
#include <optional>
#include <iterator>
#include <algorithm>
#include <utility>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/field.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .vtu file format
 */
class VTUReader : public GridReader {
 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "UnstructuredGrid");

        _num_points = from_string<std::size_t>(helper.get("UnstructuredGrid/Piece").get_attribute("NumberOfPoints"));
        _num_cells = from_string<std::size_t>(helper.get("UnstructuredGrid/Piece").get_attribute("NumberOfCells"));
        VTK::XMLDetail::copy_field_names_from(helper.get("UnstructuredGrid"), fields);
        _helper.emplace(std::move(helper));
    }

    void _close() override {
        _helper.reset();
        _num_points = 0;
        _num_cells = 0;
    }

    std::string _name() const override {
        return "VTUReader";
    }

    std::size_t _number_of_cells() const override {
        return _num_cells;
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
        return _helper.value().make_points_field("UnstructuredGrid/Piece/Points", _number_of_points());
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        const auto types = _helper.value().make_data_array_field(
            "types", "UnstructuredGrid/Piece/Cells", _number_of_cells()
        )->template export_to<std::vector<std::uint_least8_t>>();
        const auto offsets = _helper.value().make_data_array_field(
            "offsets", "UnstructuredGrid/Piece/Cells", _number_of_cells()
        )->template export_to<std::vector<std::size_t>>();
        const auto connectivity = _helper.value().make_data_array_field(
            "connectivity", "UnstructuredGrid/Piece/Cells"
        )->template export_to<std::vector<std::size_t>>();
        std::vector<std::size_t> corners;
        for (std::size_t i = 0; i < _number_of_cells(); ++i) {
            corners.clear();
            const std::size_t offset_begin = (i == 0 ? 0 : offsets.at(i-1));
            const std::size_t offset_end = offsets.at(i);
            if (connectivity.size() < offset_end)
                throw SizeError("Connectivity array read from the file is too small");
            if (offset_end < offset_begin)
                throw ValueError("Invalid offset array");
            std::copy_n(connectivity.begin() + offset_begin, offset_end - offset_begin, std::back_inserter(corners));
            visitor(VTK::cell_type(types.at(i)), corners);
        }
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "UnstructuredGrid/Piece/CellData", _number_of_cells());
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "UnstructuredGrid/Piece/PointData", _number_of_points());
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "UnstructuredGrid/FieldData");
    }

    std::optional<VTK::XMLReaderHelper> _helper;
    std::size_t _num_points;
    std::size_t _num_cells;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTU_READER_HPP_
