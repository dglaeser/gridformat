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
#include <gridformat/common/md_index.hpp>
#include <gridformat/common/flat_index_mapper.hpp>

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
        requires(!std::is_same_v<std::remove_cvref_t<C>, NullCommunicator>)
    explicit VTKHDFImageGridReader(C&&) {
        static_assert(false, "Cannot read vtk-hdf image grid files in parallel");
    }

 private:
    std::string _name() const override {
        return "VTKHDFImageGridReader";
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& field_names) override {
        _file = HDF5File{filename, _comm, HDF5File::read_only};

        const auto type = VTKHDF::get_file_type(_file.value());
        if (type != "ImageData")
            throw ValueError("Incompatible VTK-HDF type: '" + type + "', expected 'ImageData'.");

        if (_file->exists("VTKHDF/Steps"))
            _file->visit_attribute("VTKHDF/Steps/NSteps", [&] (auto&& field) {
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
        _cell_extents = {extents[1], extents[3], extents[5]};

        _make_grid_dim_to_space_dim_map();
    }

    void _close() override {
        _file = {};
        _cell_extents = {};
        _cell_spacing = {};
        _point_origin = {};
        _direction = {};
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

    auto _point_extents() const {
        return _cell_extents | std::views::transform([] (std::size_t e) {
            return e == 0 ? 0 : e + 1;
        });
    }

    template<std::ranges::range R>
    auto _actives(R&& r) const {
        return r | std::views::filter([] (auto e) { return e != 0; });
    }

    std::array<std::size_t, 3> _extents() const override {
        return _cell_extents;
    }

    std::array<double, 3> _spacing() const override {
        return _cell_spacing;
    }

    std::array<double, 3> _origin() const override {
        return _point_origin;
    }

    std::size_t _number_of_cells() const override {
        auto actives = _actives(_cell_extents);
        return std::accumulate(
            std::ranges::begin(actives),
            std::ranges::end(actives),
            std::size_t{1},
            std::multiplies{}
        );
    }

    std::size_t _number_of_points() const override {
        auto actives = _actives(_point_extents());
        return std::accumulate(
            std::ranges::begin(actives),
            std::ranges::end(actives),
            std::size_t{1},
            std::multiplies{}
        );
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        const MDLayout point_layout{_actives(_point_extents())};
        const FlatIndexMapper point_mapper{point_layout};
        std::vector<std::size_t> corners;
        for (const auto& md_index : MDIndexRange{MDLayout{_actives(_cell_extents)}}) {
            const auto p0 = point_mapper.map(md_index);
            if (point_layout.dimension() == 1) {
                corners.resize(2);
                corners = {p0, p0 + 1};
                visitor(CellType::segment, corners);
            } else if (point_layout.dimension() == 2) {
                const auto x_offset = point_layout.extent(0);
                corners.resize(4);
                corners = {p0, p0 + 1, p0 + x_offset, p0 + 1 + x_offset};
                visitor(CellType::pixel, corners);
            } else {
                const auto x_offset = point_layout.extent(0);
                const auto y_offset = point_layout.number_of_entries(0)*point_layout.number_of_entries(1);
                corners.resize(4);
                corners = {
                    p0, p0 + 1, p0 + x_offset, p0 + 1 + x_offset,
                    p0 + y_offset, p0 + y_offset + 1, p0 + y_offset + x_offset, p0 + 1 + y_offset + x_offset
                };
                visitor(CellType::voxel, corners);
            }
        }
    }

    FieldPtr _points() const override {
        const MDLayout point_layout{_actives(_point_extents())};
        const FlatIndexMapper point_mapper{point_layout};

        std::vector<double> buffer(_number_of_points()*vtk_space_dim, 0.0);
        for (const auto& md_index : MDIndexRange{point_layout}) {
            const auto offset = point_mapper.map(md_index)*vtk_space_dim;
            std::copy_n(_point_origin.begin(), vtk_space_dim, buffer.data() + offset);
            for (std::size_t i = 0; i < _grid_dimension(); ++i) {
                const auto space_dir = _grid_to_space_dim_map.at(i);
                const double deltaX = md_index.get(i)*_cell_spacing.at(space_dir);
                const auto basis_vec = _direction
                    | std::views::drop(space_dir*vtk_space_dim)
                    | std::views::take(vtk_space_dim);
                std::ranges::for_each(basis_vec, [&, j=0] (double bij) mutable {
                    buffer.at(offset + j++) += bij*deltaX;
                });
            }
        }

        return make_field_ptr(BufferField{std::move(buffer), MDLayout{{_number_of_points(), vtk_space_dim}}});
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

    std::optional<std::size_t> _get_number_of_components(std::string_view path) const {
        const auto layout = _file.value().get_dimensions(std::string{path}).value();
        const std::size_t step_offset = _is_transient() ? 1 : 0;
        if (layout.size() == _grid_dimension() + step_offset + 1)
            return layout.back();
        return {};
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

    void _make_grid_dim_to_space_dim_map() {
        const auto actives = Ranges::to_array<vtk_space_dim>(
            _cell_extents | std::views::transform([] (auto e) { return e != 0; })
        );

        std::array<unsigned int, vtk_space_dim> zero_count;
        for (unsigned int i = 0; i < vtk_space_dim; ++i)
            zero_count[i] = std::accumulate(actives.begin(), actives.begin() + i, 0);

        _grid_to_space_dim_map.clear();
        for (unsigned int i = 0; i < vtk_space_dim; ++i)
            if (actives[i])
                _grid_to_space_dim_map.push_back(zero_count[i]);
    }

    std::size_t _grid_dimension() const {
        return _grid_to_space_dim_map.size();
    }

    bool _is_transient() const {
        return static_cast<bool>(_num_steps);
    }

    Communicator _comm;
    std::optional<HDF5File> _file;
    std::array<std::size_t, 3> _cell_extents;
    std::array<double, 3> _cell_spacing;
    std::array<double, 9> _direction;
    std::array<double, 3> _point_origin;
    std::vector<std::size_t> _grid_to_space_dim_map;
    std::optional<std::size_t> _num_steps;
    std::optional<std::size_t> _step_index;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_IMAGE_GRID_READER_HPP_
