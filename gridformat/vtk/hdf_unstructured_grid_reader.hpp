// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Reader for the VTK HDF file format for unstructured grids.
 */
#ifndef GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_READER_HPP_
#define GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_READER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <iterator>
#include <optional>
#include <algorithm>
#include <concepts>
#include <numeric>
#include <utility>
#include <cassert>

#include <gridformat/common/field.hpp>
#include <gridformat/common/field_transformations.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/grid/reader.hpp>
#include <gridformat/parallel/concepts.hpp>
#include <gridformat/vtk/hdf_common.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief Reader for the VTK-HDF file format for unstructured grids.
 */
template<Concepts::Communicator Communicator = NullCommunicator>
class VTKHDFUnstructuredGridReader : public GridReader {
    using HDF5File = HDF5::File<Communicator>;
    static constexpr std::size_t vtk_space_dim = 3;

 public:
    explicit VTKHDFUnstructuredGridReader() requires(std::is_same_v<Communicator, NullCommunicator>)
    : _comm{}
    {}

    explicit VTKHDFUnstructuredGridReader(Communicator comm)
    : _comm{comm}
    {}

 private:
    std::string _name() const override {
        return "VTKHDFUnstructuredGridReader";
    }

    void _open(const std::string& filename, typename GridReader::FieldNames& field_names) override {
        _file = HDF5File{filename, _comm, HDF5File::read_only};

        const auto type = VTKHDF::get_file_type(_file.value());
        if (type != "UnstructuredGrid")
            throw ValueError("Incompatible VTK-HDF type: '" + type + "', expected 'UnstructuredGrid'.");

        VTKHDF::check_version_compatibility(_file.value(), {2, 0});

        if (_file->exists("VTKHDF/Steps"))
            _file->visit_attribute("VTKHDF/Steps/NSteps", [&] (auto&& field) {
                _num_steps.emplace();
                field.export_to(_num_steps.value());
                _step_index.emplace(0);
            });

        _compute_piece_offsets();
        const auto copy_names_in = [&] (const std::string& group, auto& storage) {
            if (_file->exists(group))
                std::ranges::copy(_file->dataset_names_in(group), std::back_inserter(storage));
        };
        copy_names_in("VTKHDF/CellData", field_names.cell_fields);
        copy_names_in("VTKHDF/PointData", field_names.point_fields);
        copy_names_in("VTKHDF/FieldData", field_names.meta_data_fields);
    }

    void _close() override {
        _file = {};
        _num_cells = 0;
        _num_points = 0;
    }

    void _compute_piece_offsets() {
        _cell_offset = 0;
        _point_offset = 0;
        _num_cells = _read_rank_scalar<std::size_t>("/VTKHDF/NumberOfCells", _get_part_offset());
        _num_points = _read_rank_scalar<std::size_t>("/VTKHDF/NumberOfPoints", _get_part_offset());

        if (Parallel::size(_comm) > 1) {
            _check_communicator_size();
            const auto proc_cells = Parallel::broadcast(_comm, Parallel::gather(_comm, _num_cells));
            const auto proc_points = Parallel::broadcast(_comm, Parallel::gather(_comm, _num_points));
            const auto compute_offset = [&] (const auto& v) {
                return std::accumulate(v.begin(), v.begin() + Parallel::rank(_comm), std::size_t{0});
            };
            _cell_offset += compute_offset(proc_cells);
            _point_offset += compute_offset(proc_points);
        }
    }

    std::size_t _get_part_offset() const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "/VTKHDF/Steps/PartOffsets",
                HDF5::Slice{
                    .offset = {_step_index.value()},
                    .count = {1}
                })
            : std::size_t{0};
    }

    std::size_t _get_cells_offset() const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/CellOffsets",
                HDF5::Slice{
                    .offset = {_step_index.value(), 0},
                    .count = {1, 1}
                })
            : std::size_t{0};
    }

    std::size_t _get_points_offset() const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/PointOffsets",
                HDF5::Slice{
                    .offset = {_step_index.value()},
                    .count = {1}
                })
            : std::size_t{0};
    }

    std::size_t _get_connectivity_id_offset() const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/ConnectivityIdOffsets",
                    HDF5::Slice{
                    .offset = {_step_index.value(), 0},
                    .count = {1, 1}
                })
            : std::size_t{0};
    }

    std::size_t _get_cell_data_offset(std::string_view name) const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/CellDataOffsets/" + std::string{name},
                HDF5::Slice{
                    .offset = {_step_index.value()},
                    .count = {1}
                })
            : std::size_t{0};
    }

    std::size_t _get_point_data_offset(std::string_view name) const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/PointDataOffsets/" + std::string{name},
                HDF5::Slice{
                    .offset = {_step_index.value()},
                    .count = {1}
                })
            : std::size_t{0};
    }

    std::size_t _get_field_data_offset(std::string_view name) const {
        return _is_transient()
            ? _access_file().template read_dataset_to<std::size_t>(
                "VTKHDF/Steps/FieldDataOffsets/" + std::string{name},
                HDF5::Slice{
                    .offset = {_step_index.value()},
                    .count = {1}
                })
            : std::size_t{0};
    }

    bool _is_sequence() const override {
        return _is_transient();
    }

    std::size_t _number_of_steps() const override {
        if (!_num_steps)
            throw ValueError("No step information available");
        return _num_steps.value();
    }

    double _time_at_step(std::size_t step_idx) const override {
        if (step_idx >= _number_of_steps())
            throw ValueError("Only " + as_string(_number_of_steps()) + " available");
        return _access_file().template read_dataset_to<double>("/VTKHDF/Steps/Values", HDF5::Slice{
            .offset = {step_idx},
            .count = {1}
        });
    }

    void _set_step(std::size_t step_idx, typename GridReader::FieldNames&) override {
        if (step_idx >= _number_of_steps())
            throw ValueError("Only " + as_string(_number_of_steps()) + " available");
        if (step_idx != _step_index.value()) {
            _compute_piece_offsets();
            _step_index = step_idx;
        }
    }

    std::size_t _number_of_cells() const override {
        return _num_cells;
    }

    std::size_t _number_of_points() const override {
        return _num_points;
    }

    std::size_t _number_of_pieces() const override {
        if (Parallel::size(_comm) > 1)
            return _number_of_pieces_in_file();
        return 1;
    }

    std::size_t _number_of_pieces_in_file() const {
        using V = std::vector<std::size_t>;
        return _is_transient()
            ? _access_file().template read_dataset_to<V>("/VTKHDF/Steps/NumberOfParts").at(_step_index.value())
            : _access_file().template read_dataset_to<V>("/VTKHDF/NumberOfCells").size();
    }

    void _check_communicator_size() const {
        const auto nprocs = _number_of_pieces_in_file();
        if (nprocs != static_cast<std::size_t>(Parallel::size(_comm)))
            throw SizeError(
                "Can only read the file in parallel if the size of the communicator matches the size "
                "of that used when writing the file. Please read in the file sequentially on one process "
                "and distribute the grid yourself, or restart the parallel run with "
                + std::to_string(nprocs) + " processes."
            );
    }

    void _visit_cells(const typename GridReader::CellVisitor& visitor) const override {
        const auto num_connectivity_ids = _file.value().template read_dataset_to<std::vector<std::size_t>>(
            "VTKHDF/NumberOfConnectivityIds",
            HDF5::Slice{
                .offset = {_get_part_offset()},
                .count = {static_cast<std::size_t>(Parallel::size(_comm))}
            }
        );
        const auto my_num_connectivity_ids = num_connectivity_ids.at(Parallel::rank(_comm));
        const auto my_id_offset = _get_connectivity_id_offset() + std::accumulate(
            num_connectivity_ids.begin(),
            num_connectivity_ids.begin() + Parallel::rank(_comm),
            std::size_t{0}
        );
        auto offsets = _file.value().template read_dataset_to<std::vector<std::size_t>>(
            "VTKHDF/Offsets",
            HDF5::Slice{
                // offsets have length num_cells+1, so we need to add +1 per rank to the cell offsets
                .offset = {
                    _get_cells_offset() + _cell_offset
                    + Parallel::size(_comm)*_step_index.value_or(0) +
                    + Parallel::rank(_comm)
                },
                .count = {_num_cells + 1}
            }
        );
        auto types = _file.value().template read_dataset_to<std::vector<std::uint_least8_t>>(
            "VTKHDF/Types",
            HDF5::Slice{.offset = {_get_cells_offset() + _cell_offset}, .count = {_num_cells}}
        );
        auto connectivity = _file.value().template read_dataset_to<std::vector<std::size_t>>(
            "VTKHDF/Connectivity",
            HDF5::Slice{.offset = {my_id_offset}, .count = {my_num_connectivity_ids}}
        );

        std::vector<std::size_t> corners;
        for (std::size_t i = 0; i < types.size(); ++i) {
            assert(offsets.at(i+1) > offsets.at(i));
            const auto num_corners = offsets.at(i+1) - offsets.at(i);
            corners.resize(num_corners);
            for (std::size_t c = 0; c < num_corners; ++c)
                corners[c] = connectivity.at(offsets.at(i) + c);
            visitor(VTK::cell_type(types.at(i)), corners);
        }
    }

    FieldPtr _points() const override {
        const std::string path = "VTKHDF/Points";
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            MDLayout{{_num_points, vtk_space_dim}},
            _file.value().get_precision(path).value(),
            _serialization_callback(path, HDF5::Slice{
                .offset = {_point_offset + _get_points_offset(), 0},
                .count = {_num_points, vtk_space_dim}
            })
        });
    }

    FieldPtr _cell_field(std::string_view name) const override {
        const std::string path = "VTKHDF/CellData/" + std::string{name};
        const auto slice = _get_slice(path, _num_cells, _cell_offset + _get_cell_data_offset(name));
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            MDLayout{slice.count},
            _file.value().get_precision(path).value(),
            _serialization_callback(path, slice)
        });
    }

    FieldPtr _point_field(std::string_view name) const override {
        const std::string path = "VTKHDF/PointData/" + std::string{name};
        const auto slice = _get_slice(path, _num_points, _point_offset + _get_point_data_offset(name));
        return make_field_ptr(VTKHDF::DataSetField{
            _file.value(),
            MDLayout{slice.count},
            _file.value().get_precision(path).value(),
            _serialization_callback(path, slice)
        });
    }

    FieldPtr _meta_data_field(std::string_view name) const override {
        const auto path = "VTKHDF/FieldData/" + std::string{name};
        const auto dims = _file.value().get_dimensions(path).value();
        if (dims.size() == 1)
            return make_field_ptr(VTKHDF::DataSetField{_file.value(), path});
        if (dims.size() != 2)
            throw SizeError("Unexpected field data array size");
        auto offset = dims;
        auto count = offset;
        count.at(0) = 1;
        std::ranges::fill(offset, 0);
        offset.at(0) = _get_field_data_offset(name);
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

    template<Concepts::Scalar T>
    T _read_rank_scalar(const std::string& path, const std::size_t base_offset = 0) const {
        T result;
        _file.value().visit_dataset(
            path,
            [&] (auto&& field) { field.export_to(result); },
            HDF5::Slice{
                .offset = {base_offset + static_cast<std::size_t>(Parallel::rank(_comm))},
                .count = {1}
            }
        );
        return result;
    }

    HDF5::Slice _get_slice(const std::string& path,
                           std::size_t count,
                           std::size_t offset) const {
        auto ds_size = _file.value().get_dimensions(path).value();
        auto ds_offset = ds_size;
        std::ranges::fill(ds_offset, std::size_t{0});
        ds_size.at(0) = count;
        ds_offset.at(0) = offset;
        return {.offset = ds_offset, .count = ds_size};
    }

    auto _serialization_callback(std::string path, std::optional<HDF5::Slice> slice = {}) const {
        return [_p=std::move(path), _s=std::move(slice)] (const HDF5File& file) {
            if (_s)
                return file.visit_dataset(_p, [&] <typename F> (F&& f) { return f.serialized(); }, _s.value());
            return file.visit_dataset(_p, [&] <typename F> (F&& f) { return f.serialized(); });
        };
    }

    bool _is_transient() const {
        return static_cast<bool>(_num_steps);
    }

    const HDF5File& _access_file() const {
        if (!_file.has_value())
            throw InvalidState("No file has been read");
        return _file.value();
    }

    Communicator _comm;
    std::optional<HDF5File> _file;
    std::size_t _num_cells;
    std::size_t _num_points;
    std::size_t _cell_offset;
    std::size_t _point_offset;
    std::optional<std::size_t> _num_steps;
    std::optional<std::size_t> _step_index;
};

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_READER_HPP_
