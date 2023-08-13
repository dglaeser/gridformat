// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Reader for the VTK HDF file format for image grids.
 */
#ifndef GRIDFORMAT_VTK_HDF_IMAGE_GRID_READER_HPP_
#define GRIDFORMAT_VTK_HDF_IMAGE_GRID_READER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <iterator>
#include <optional>
#include <algorithm>
#include <functional>
#include <concepts>
#include <numeric>
#include <utility>
#include <ranges>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/lazy_field.hpp>
#include <gridformat/common/ranges.hpp>

#include <gridformat/grid/reader.hpp>
#include <gridformat/parallel/concepts.hpp>
#include <gridformat/vtk/hdf_common.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for the VTK-HDF file format for image grids.
 */
class VTKHDFImageGridReader : public GridReader {
    using Communicator = NullCommunicator;
    using HDF5File = HDF5::File<Communicator>;
    static constexpr std::size_t vtk_space_dim = 3;

 public:
    explicit VTKHDFImageGridReader() = default;

    template<Concepts::Communicator C>
    explicit VTKHDFImageGridReader(C&&) {
        static_assert(
            std::is_same_v<std::remove_cvref_t<C>, NullCommunicator>,
            "Cannot read vtk-hdf image grid files in parallel"
        );
    }

 private:
    std::string _name() const override {
        if (_is_transient())
            return "VTKHDFImageGridReader (transient)";
        return "VTKHDFImageGridReader";
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& field_names) override {
        _file = HDF5File{filename, _comm, HDF5File::read_only};

        const auto type = VTKHDF::get_file_type(_file.value());
        if (type != "ImageData")
            throw ValueError("Incompatible VTK-HDF type: '" + type + "', expected 'ImageData'.");

        VTKHDF::check_version_compatibility(_file.value(), {2, 0});

        if (_file->exists("/VTKHDF/Steps"))
            _file->visit_attribute("/VTKHDF/Steps/NSteps", [&] (auto&& field) {
                _num_steps.emplace();
                field.export_to(_num_steps.value());
                _step_index.emplace(0);
            });

        const auto copy_names_in = [&] (const std::string& group, auto& storage) {
            if (_file->exists(group))
                std::ranges::copy(_file->dataset_names_in(group), std::back_inserter(storage));
        };
        copy_names_in("VTKHDF/CellData", field_names.cell_fields);
        copy_names_in("VTKHDF/PointData", field_names.point_fields);
        copy_names_in("VTKHDF/FieldData", field_names.meta_data_fields);

        auto spacing = _file.value().template read_attribute_to<std::vector<double>>("/VTKHDF/Spacing");
        if (spacing.size() != vtk_space_dim)
            throw SizeError("Unexpected spacing vector read (size = " + as_string(spacing.size()) + ")");
        _cell_spacing = Ranges::to_array<vtk_space_dim>(spacing);

        auto direction = std::vector<double>{1., 0., 0., 0., 1., 0., 0., 0., 1.};
        if (_file->has_attribute_at("/VTKHDF/Direction"))
            direction = _file->template read_attribute_to<std::vector<double>>("/VTKHDF/Direction");
        if (direction.size() != 9)
            throw SizeError("Unexpected direction vector read (size = " + as_string(direction.size()) + ")");
        _direction = Ranges::to_array<9>(direction);

        auto origin = std::vector<double>{0., 0., 0.};
        if (_file->has_attribute_at("/VTKHDF/Origin"))
            origin = _file->template read_attribute_to<std::vector<double>>("/VTKHDF/Origin");
        if (origin.size() != vtk_space_dim)
            throw SizeError("Unexpected origin read (size = " + as_string(origin.size()) + ")");
        _point_origin = Ranges::to_array<vtk_space_dim>(origin);

        auto extents = _file.value().template read_attribute_to<std::vector<std::size_t>>("/VTKHDF/WholeExtent");
        if (extents.size() != 6)
            throw SizeError("Unexpected 'WholeExtents' attribute (size = " + as_string(extents.size()) + ").");
        _piece_location.lower_left = {extents[0], extents[2], extents[4]};
        _piece_location.upper_right = {extents[1], extents[3], extents[5]};
    }

    void _close() override {
        _file = {};
        _piece_location = {};
        _cell_spacing = {};
        _point_origin = {};
        _direction = {};
    }

    typename GridReader::PieceLocation _location() const override {
        return _piece_location;
    }

    typename GridReader::Vector _spacing() const override {
        return _cell_spacing;
    }

    typename GridReader::Vector _origin() const override {
        return _point_origin;
    }

    std::size_t _number_of_cells() const override {
        const auto counts = _get_extents();
        auto actives = counts | std::views::filter([] (auto e) { return e != 0; });
        return std::accumulate(
            std::ranges::begin(actives),
            std::ranges::end(actives),
            std::size_t{1},
            std::multiplies{}
        );
    }

    std::size_t _number_of_points() const override {
        auto counts = Ranges::incremented(_get_extents(), 1);
        return std::accumulate(
            std::ranges::begin(counts),
            std::ranges::end(counts),
            std::size_t{1},
            std::multiplies{}
        );
    }

    std::size_t _number_of_pieces() const override {
        return 1;
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        VTK::CommonDetail::visit_structured_cells(visitor, _make_vtk_extents_array());
    }

    FieldPtr _points() const override {
        std::array<std::size_t, 6> extents = _make_vtk_extents_array();
        extents[1] += 1;
        extents[3] += 1;
        extents[5] += 1;
        return make_field_ptr(LazyField{
            int{},  // dummy source
            MDLayout{{VTK::CommonDetail::number_of_entities(extents), std::size_t{vtk_space_dim}}},
            Precision<double>{},
            [ex=extents, o=_point_origin, s=_cell_spacing, d=_direction] (const int&) {
                return VTK::CommonDetail::serialize_structured_points(ex, o, s, d);
            }
        });
    }

    bool _is_sequence() const override {
        return _is_transient();
    }

    std::size_t _number_of_steps() const override {
        if (!_num_steps.has_value())
            throw ValueError("Read file is not a sequence");
        return _num_steps.value();
    }

    double _time_at_step(std::size_t step_idx) const override {
        if (step_idx >= _number_of_steps())
            throw ValueError("Only " + as_string(_number_of_steps()) + " available");
        return _file.value().template read_dataset_to<double>("/VTKHDF/Steps/Values", HDF5::Slice{
            .offset = {step_idx},
            .count = {1}
        });
    }

    void _set_step(std::size_t step_idx, typename GridReader::FieldNames&) override {
        if (step_idx >= _number_of_steps())
            throw ValueError("Only " + as_string(_number_of_steps()) + " available");
        _step_index = step_idx;
    }

    auto _serialization_callback(std::string path, std::size_t target_size) const {
        return [&, size=target_size, _step=_step_index, _p=std::move(path)] (const HDF5File& file) {
            auto count = file.get_dimensions(_p).value();
            auto offset = count;
            std::ranges::fill(offset, 0);
            if (_step) {
                count.at(0) = 1;
                offset.at(0) = _step.value();
            }
            return file.visit_dataset(_p, [&] <typename F> (F&& field) {
                const std::size_t step_offset = _step.has_value() ? 1 : 0;
                const auto layout = field.layout();
                if (layout.dimension() == _grid_dimension() + step_offset)
                    return transform(make_field_ptr(std::move(field)), FieldTransformation::reshape_to(
                        MDLayout{{size}}
                    ))->serialized();
                else if (layout.dimension() == _grid_dimension() + step_offset + 1)
                    return transform(make_field_ptr(std::move(field)), FieldTransformation::reshape_to(
                        MDLayout{{size, layout.extent(_grid_dimension() + step_offset)}}
                    ))->serialized();
                throw InvalidState("Unexpected field layout: " + as_string(layout));
            }, HDF5::Slice{.offset = offset, .count = count});
        };
    }

    FieldPtr _cell_field(std::string_view name) const override {
        const std::string path = "VTKHDF/CellData/" + std::string{name};
        const auto num_components = _get_number_of_components(path);
        const auto layout = num_components ? MDLayout{{_number_of_cells(), num_components.value()}}
                                           : MDLayout{{_number_of_cells()}};
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            std::move(layout),
            _file.value().get_precision(path).value(),
            _serialization_callback(path, _number_of_cells())
        });
    }

    FieldPtr _point_field(std::string_view name) const override {
        const std::string path = "VTKHDF/PointData/" + std::string{name};
        const auto num_components = _get_number_of_components(path);
        const auto layout = num_components ? MDLayout{{_number_of_points(), num_components.value()}}
                                           : MDLayout{{_number_of_points()}};
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            std::move(layout),
            _file.value().get_precision(path).value(),
            _serialization_callback(path, _number_of_points())
        });
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        const auto path = "VTKHDF/FieldData/" + std::string{name};
        const auto dims = _file.value().get_dimensions(path).value();
        if (dims.size() == 1)
            return make_field_ptr(VTKHDF::DataSetField{_file.value(), path});
        if (dims.size() > 3 || (!_is_transient() && dims.size() != 2))
            throw SizeError("Unexpected field data array size");
        if (dims.at(dims.size() - 2) != 1)
            throw SizeError("Cannot only read one-dimensional field data");

        auto offset = dims;
        auto count = offset;
        std::ranges::fill(offset, 0);
        count.at(0) = 1;
        offset.at(0) = _is_transient() ? _step_index.value() : 0;
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            MDLayout{count | std::views::drop(1)},
            _file.value().get_precision(path).value(),
            [p=path, o=offset, c=count] (const HDF5File& f) {
                return f.visit_dataset(p, [&] <typename F> (F&& field) {
                    return FlattenedField{make_field_ptr(std::move(field))}.serialized();
                }, HDF5::Slice{.offset = o, .count = c});
            }
        });
    }

    std::array<std::size_t, 6> _make_vtk_extents_array() const {
        std::array<std::size_t, 6> extents;
        extents[0] = _piece_location.lower_left[0];
        extents[2] = _piece_location.lower_left[1];
        extents[4] = _piece_location.lower_left[2];
        extents[1] = _piece_location.upper_right[0];
        extents[3] = _piece_location.upper_right[1];
        extents[5] = _piece_location.upper_right[2];
        return extents;
    }

    std::optional<std::size_t> _get_number_of_components(std::string_view path) const {
        const auto layout = _file.value().get_dimensions(std::string{path}).value();
        const std::size_t step_offset = _is_transient() ? 1 : 0;
        if (layout.size() == _grid_dimension() + step_offset + 1)
            return layout.back();
        return {};
    }

    std::size_t _grid_dimension() const {
        return VTK::CommonDetail::structured_grid_dimension(_get_extents());
    }

    std::array<std::size_t, 3> _get_extents() const {
        return {
            _piece_location.upper_right[0] - _piece_location.lower_left[0],
            _piece_location.upper_right[1] - _piece_location.lower_left[1],
            _piece_location.upper_right[2] - _piece_location.lower_left[2]
        };
    }

    bool _is_transient() const {
        return static_cast<bool>(_num_steps);
    }

    Communicator _comm;
    std::optional<HDF5File> _file;
    typename GridReader::PieceLocation _piece_location;
    std::array<double, 9> _direction;
    std::array<double, 3> _cell_spacing;
    std::array<double, 3> _point_origin;
    std::optional<std::size_t> _num_steps;
    std::optional<std::size_t> _step_index;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_IMAGE_GRID_READER_HPP_
