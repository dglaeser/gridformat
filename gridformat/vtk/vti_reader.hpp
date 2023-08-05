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

        VTK::XMLDetail::copy_field_names_from(helper.get("ImageData"), fields);
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
        return VTK::CommonDetail::number_of_entities(_point_extents());
    }

    bool _is_sequence() const override {
        return false;
    }

    FieldPtr _points() const override {
        const auto pextents = _point_extents();
        const auto num_points = VTK::CommonDetail::number_of_entities(_point_extents());
        return make_field_ptr(LazyField{
            int{},  // dummy "source"
            MDLayout{{num_points, std::size_t{3}}},
            Precision<double>{},
            [specs=_specs(), pextents=pextents, n=num_points] (const int&) {
                return VTK::CommonDetail::serialize_structured_points(
                    pextents,
                    specs.origin,
                    specs.spacing,
                    specs.direction
                );
            }
        });
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        VTK::CommonDetail::visit_structured_cells(visitor, _specs().extents);
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

    std::array<std::size_t, 6> _point_extents() const {
        auto result = _specs().extents;
        result[1] += 1;
        result[3] += 1;
        result[5] += 1;
        return result;
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
