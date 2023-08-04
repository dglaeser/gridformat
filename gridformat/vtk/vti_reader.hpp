// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTIReader
 */
#ifndef GRIDFORMAT_VTK_VTI_READER_HPP_
#define GRIDFORMAT_VTK_VTI_READER_HPP_

#include <string>
#include <optional>
#include <iterator>
#include <sstream>
#include <ranges>
#include <array>
#include <cmath>

#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/lazy_field.hpp>

#include <gridformat/grid/cell_type.hpp>
#include <gridformat/grid/reader.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .vti file format
 */
class VTIReader : public GridReader {
 private:
    struct ImageSpecs {
        std::array<std::size_t, 6> extents;
        std::array<double, 3> spacing;
        std::array<double, 3> origin;
        std::array<double, 9> direction;
    };

    template<typename T, std::size_t N>
    std::array<T, N> _array_from_string(const std::string& values) const {
        auto result = Ranges::filled_array<N>(T{0});
        std::stringstream stream;
        stream << values;
        auto stream_view = std::ranges::istream_view<T>(stream);
        const auto [_, out] = std::ranges::copy(stream_view, result.begin());
        if (std::ranges::distance(result.begin(), out) != N)
            throw IOError("Could not read " + std::to_string(N) + " values from '" + values + "'");
        return result;
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "ImageData");
        auto specs = ImageSpecs{};
        specs.extents = _array_from_string<std::size_t, 6>(helper.get("ImageData/Piece").get_attribute("Extent"));
        specs.spacing = _array_from_string<double, 3>(helper.get("ImageData").get_attribute("Spacing"));
        specs.origin = _array_from_string<double, 3>(helper.get("ImageData").get_attribute("Origin"));
        specs.direction = _array_from_string<double, 9>(helper.get("ImageData").get_attribute_or(
            std::string{"1 0 0 0 1 0 0 0 1"}, "Direction"
        ));

        if (helper.get("ImageData/Piece").has_child("PointData"))
            std::ranges::copy(
                helper.data_arrays_at("ImageData/Piece/PointData")
                    | std::views::transform([] (const auto& data_array) { return data_array.get_attribute("Name"); }),
                std::back_inserter(fields.point_fields)
            );
        if (helper.get("ImageData/Piece").has_child("CellData"))
            std::ranges::copy(
                helper.data_arrays_at("ImageData/Piece/CellData")
                    | std::views::transform([] (const auto& data_array) { return data_array.get_attribute("Name"); }),
                std::back_inserter(fields.cell_fields)
            );
        if (helper.get("ImageData").has_child("FieldData"))
            std::ranges::copy(
                helper.data_arrays_at("ImageData/FieldData")
                    | std::views::transform([] (const auto& data_array) { return data_array.get_attribute("Name"); }),
                std::back_inserter(fields.meta_data_fields)
            );

        _helper.emplace(std::move(helper));
        _image_specs.emplace(std::move(specs));
    }

    void _close() override {
        _helper.reset();
        _image_specs.reset();
    }

    std::string _name() const override {
        return "VTIReader";
    }

    std::size_t _number_of_cells() const override {
        return VTK::CommonDetail::number_of_entities(_specs().extents);
    }

    std::size_t _number_of_points() const override {
        auto point_extents = _specs().extents;
        point_extents[1] += 1;
        point_extents[3] += 1;
        point_extents[5] += 1;
        return VTK::CommonDetail::number_of_entities(point_extents);
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _points() const override {
        return make_field_ptr(LazyField{
            int{},  // dummy "source"
            MDLayout{{_number_of_points(), std::size_t{3}}},
            Precision<double>{},
            [specs=_specs(), n=_number_of_points()] (const int&) {
                Serialization result(sizeof(double)*n*3);
                auto span_out = result.as_span_of(Precision<double>{});
                const std::size_t n_z = specs.extents[5] + 1 - specs.extents[4];
                const std::size_t n_y = specs.extents[3] + 1 - specs.extents[2];
                const std::size_t n_x = specs.extents[1] + 1 - specs.extents[0];
                if (n_z*n_y*n_x != n)
                    throw SizeError("Number of points doesn't match");
                for (std::size_t z = 0; z < n_z; ++z)
                    for (std::size_t y = 0; y < n_y; ++y)
                        for (std::size_t x = 0; x < n_x; ++x) {
                            const std::size_t offset = (z*n_y*n_x + y*n_x + x)*3;
                            const auto dx = x*specs.spacing[0];
                            const auto dy = y*specs.spacing[1];
                            const auto dz = z*specs.spacing[2];
                            span_out[offset + 0] += dx*specs.direction[0] + dy*specs.direction[1] + dz*specs.direction[2];
                            span_out[offset + 1] += dx*specs.direction[3] + dy*specs.direction[4] + dz*specs.direction[5];
                            span_out[offset + 2] += dx*specs.direction[6] + dy*specs.direction[7] + dz*specs.direction[8];
                        }
                return result;
            }
        });
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        const auto& specs = _specs();
        const std::size_t n_z = specs.extents[5] - specs.extents[4];
        const std::size_t n_y = specs.extents[3] - specs.extents[2];
        const std::size_t n_x = specs.extents[1] - specs.extents[0];
        const unsigned int dim = (n_x > 0) + (n_y > 0) + (n_z > 0);
        if (dim < 1 || dim > 3)
            throw ValueError("Unsupported image dimension (" + std::to_string(dim) + ")");

        std::vector<std::size_t> corners;
        if (dim == 1) {
            const std::size_t n = std::max(n_x, std::max(n_y, n_z));
            for (std::size_t x = 0; x < n; ++x) {
                corners.clear();
                corners = {x, x+1};
                visitor(CellType::segment, corners);
            }
        } else if (dim == 2) {
            const std::size_t nx = (n_x == 0 ? n_y : n_x);
            const std::size_t ny = (n_x == 0 || n_y == 0 ? n_z : n_y);
            for (std::size_t y = 0; y < ny; ++y)
                for (std::size_t x = 0; x < nx; ++x) {
                    corners.clear();
                    const std::size_t pidx_offset = y*(nx+1) + x;
                    const std::size_t layer_offset = nx + 1;
                    corners = {
                        pidx_offset,
                        pidx_offset + 1,
                        pidx_offset + 1 + layer_offset,
                        pidx_offset + layer_offset
                    };
                    visitor(CellType::pixel, corners);
                }
        } else {
            for (std::size_t z = 0; z < n_z; ++z)
                for (std::size_t y = 0; y < n_y; ++y)
                    for (std::size_t x = 0; x < n_x; ++x) {
                        corners.clear();
                        const std::size_t y_layer_offset = (n_x + 1);
                        const std::size_t z_layer_offset = (n_y + 1)*(n_x + 1);
                        const std::size_t pidx_offset = z*z_layer_offset + y*y_layer_offset + x;
                        corners = {
                            pidx_offset,
                            pidx_offset + 1,
                            pidx_offset + 1 + y_layer_offset,
                            pidx_offset + y_layer_offset,
                        };
                        corners.resize(corners.size() + 4);
                        corners[5] = corners[0] + z_layer_offset;
                        corners[6] = corners[1] + z_layer_offset;
                        corners[7] = corners[2] + z_layer_offset;
                        corners[8] = corners[3] + z_layer_offset;
                        visitor(CellType::voxel, corners);
                    }
        }
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "ImageData/Piece/CellData", _number_of_cells());
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "ImageData/Piece/PointData", _number_of_points());
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "ImageData/FieldData");
    }

    const ImageSpecs& _specs() const {
        if (!_image_specs.has_value())
            throw ValueError("No data read");
        return _image_specs.value();
    }

    std::optional<VTK::XMLReaderHelper> _helper;
    std::optional<ImageSpecs> _image_specs;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTI_READER_HPP_
