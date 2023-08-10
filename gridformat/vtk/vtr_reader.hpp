// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \copydoc GridFormat::VTRReader
 */
#ifndef GRIDFORMAT_VTK_VTR_READER_HPP_
#define GRIDFORMAT_VTK_VTR_READER_HPP_

#include <string>
#include <optional>
#include <algorithm>
#include <utility>
#include <array>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/ranges.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/lazy_field.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/xml.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for .vtr file format
 */
class VTRReader : public GridReader {
 private:
    void _open(const std::string& filename, typename GridReader::FieldNames& fields) override {
        auto helper = VTK::XMLReaderHelper::make_from(filename, "RectilinearGrid");
        _extents = Ranges::array_from_string<std::size_t, 6>(helper.get("RectilinearGrid/Piece").get_attribute("Extent"));
        VTK::XMLDetail::copy_field_names_from(helper.get("RectilinearGrid"), fields);
        _helper.emplace(std::move(helper));
    }

    void _close() override {
        _helper.reset();
        _extents.reset();
    }

    std::string _name() const override {
        return "VTRReader";
    }

    std::size_t _number_of_cells() const override {
        return VTK::CommonDetail::number_of_entities(_extents.value());
    }

    std::size_t _number_of_points() const override {
        return VTK::CommonDetail::number_of_entities(_point_extents());
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

    std::vector<double> _ordinates(unsigned int i) const override {
        std::vector<double> result;
        unsigned int direction = 0;
        const XMLElement& coords = _helper.value().get("RectilinearGrid/Piece/Coordinates");
        for (const XMLElement& da : VTK::XML::data_arrays(coords))
            if (direction++ == i) {
                FieldPtr ordinates_field = _helper.value().make_data_array_field(da);
                return ordinates_field->precision().visit([&] <typename T> (const Precision<T>& prec) {
                    auto bytes = ordinates_field->serialized();
                    auto values = bytes.as_span_of(prec);
                    result.resize(values.size());
                    std::ranges::copy(values, result.begin());
                    return result;
                });
            }
        throw IOError("Could not read ordinates in direction " + std::to_string(i));
    }


    FieldPtr _points() const override {
        const XMLElement& coordinates = _helper.value().get("RectilinearGrid/Piece/Coordinates");

        std::vector<FieldPtr> ordinates;
        std::ranges::for_each(VTK::XML::data_arrays(coordinates), [&] (const XMLElement& da) {
            ordinates.push_back(_helper.value().make_data_array_field(da));
        });

        if (ordinates.size() != 3)
            throw SizeError("Expected 3 data arrays in the 'Coordinates' section");

        auto precision = ordinates.front()->precision();
        if (!std::ranges::all_of(ordinates, [&] (const auto f) { return f->precision() == precision; }))
            throw ValueError("Coordinates must use the same scalar types");

        auto num_points = _number_of_points();
        return make_field_ptr(LazyField{
            int{},  // dummy "source"
            MDLayout{{num_points, std::size_t{3}}},
            precision,
            [ordinates=std::move(ordinates), prec=precision, np=num_points] (const int&) {
                return prec.visit([&] <typename T> (const Precision<T>& p) {
                    auto x_data = ordinates.at(0)->serialized();
                    auto y_data = ordinates.at(1)->serialized();
                    auto z_data = ordinates.at(2)->serialized();
                    if (x_data.size() == 0) { x_data.resize(sizeof(T)); x_data.as_span_of(p)[0] = T{0}; }
                    if (y_data.size() == 0) { y_data.resize(sizeof(T)); y_data.as_span_of(p)[0] = T{0}; }
                    if (z_data.size() == 0) { z_data.resize(sizeof(T)); z_data.as_span_of(p)[0] = T{0}; }

                    Serialization result(np*sizeof(T)*3);
                    auto result_span = result.as_span_of(p);
                    std::size_t i = 0;
                    for (auto z : z_data.as_span_of(p))
                        for (auto y : y_data.as_span_of(p))
                            for (auto x : x_data.as_span_of(p)) {
                                assert(i + 3 <= result_span.size());
                                result_span[i + 0] = x;
                                result_span[i + 1] = y;
                                result_span[i + 2] = z;
                                i += 3;
                            }
                    return result;
                });
            }
        });
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        VTK::CommonDetail::visit_structured_cells(visitor, _extents.value());
    }

    FieldPtr _cell_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "RectilinearGrid/Piece/CellData", _number_of_cells());
    }

    FieldPtr _point_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "RectilinearGrid/Piece/PointData", _number_of_points());
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        return _helper.value().make_data_array_field(name, "RectilinearGrid/FieldData");
    }

    std::array<std::size_t, 6> _point_extents() const {
        auto result = _extents.value();
        result[1] += 1;
        result[3] += 1;
        result[5] += 1;
        return result;
    }

    std::optional<VTK::XMLReaderHelper> _helper;
    std::optional<std::array<std::size_t, 6>> _extents;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_VTR_READER_HPP_
