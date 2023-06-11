// SPDX-FileCopyrightText: 2022 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: GPL-3.0-or-later
/*!
 * \file
 * \ingroup VTK
 * \brief Writer for the VTK HDF file format for unstructured grids.
 */
#ifndef GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_WRITER_HPP_
#define GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_WRITER_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <type_traits>
#include <ostream>

#include <gridformat/common/exceptions.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/concepts.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/parallel/concepts.hpp>

#include <gridformat/grid/concepts.hpp>
#include <gridformat/grid/writer.hpp>

#include <gridformat/vtk/common.hpp>
#include <gridformat/vtk/hdf_common.hpp>

namespace GridFormat {

/*!
 * \ingroup VTK
 * \brief TODO: Doc me
 */
template<bool is_transient,
         Concepts::UnstructuredGrid G,
         Concepts::Communicator Communicator = NullCommunicator>
class VTKHDFUnstructuredGridWriterImpl : public GridDetail::WriterBase<is_transient, G>::type {
    static constexpr int root_rank = 0;
    static constexpr std::size_t vtk_space_dim = 3;

    using CT = CoordinateType<G>;
    template<typename T> using Vector = std::array<T, vtk_space_dim>;
    template<typename T> using Tensor = std::array<T, vtk_space_dim*vtk_space_dim>;

    using DataSetSlice = VTKHDF::DataSetSlice;
    using DataSetPath = VTKHDF::DataSetPath;
    using IOContext = VTKHDF::IOContext;
    using HDF5File = VTKHDF::HDF5File<Communicator>;

    static constexpr WriterOptions writer_opts{
        .use_structured_grid_ordering = false,
        .append_null_terminator_to_strings = true
    };

    struct TimeSeriesOffsets {
        std::size_t cell_offset;
        std::size_t connectivity_offset;
        std::size_t point_offset;
    };

 public:
    using Grid = G;

    explicit VTKHDFUnstructuredGridWriterImpl(const Grid& grid)
        requires(!is_transient && std::is_same_v<Communicator, NullCommunicator>)
    : GridWriter<Grid>(grid, ".hdf", writer_opts)
    {}

    explicit VTKHDFUnstructuredGridWriterImpl(const Grid& grid, const Communicator& comm)
        requires(!is_transient && std::is_copy_constructible_v<Communicator>)
    : GridWriter<Grid>(grid, ".hdf", writer_opts)
    , _comm{comm}
    {}

    explicit VTKHDFUnstructuredGridWriterImpl(const Grid& grid,
                                              std::string filename_without_extension,
                                              VTK::HDFTransientOptions opts = {})
        requires(is_transient && std::is_same_v<Communicator, NullCommunicator>)
    : TimeSeriesGridWriter<Grid>(grid, writer_opts)
    , _comm{}
    , _timeseries_filename{std::move(filename_without_extension) + ".hdf"}
    , _transient_opts{std::move(opts)}
    {}

    explicit VTKHDFUnstructuredGridWriterImpl(const Grid& grid,
                                              const Communicator& comm,
                                              std::string filename_without_extension,
                                              VTK::HDFTransientOptions opts = {})
        requires(is_transient && std::is_copy_constructible_v<Communicator>)
    : TimeSeriesGridWriter<Grid>(grid, writer_opts)
    , _comm{comm}
    , _timeseries_filename{std::move(filename_without_extension) + ".hdf"}
    , _transient_opts{std::move(opts)}
    {}

 private:
    void _write(std::ostream&) const {
        throw InvalidState("VTKHDFUnstructuredGridWriter does not support export into stream");
    }

    void _write(const std::string& filename_with_ext) const {
        if constexpr (is_transient)
            throw InvalidState("This overload only works for non-transient output");
        HDF5File file{filename_with_ext, _comm, HDF5File::overwrite};
        _write_to(file);
    }

    std::string _write(double t) {
        if constexpr (!is_transient)
            throw InvalidState("This overload only works for transient output");

        if (this->_step_count == 0)
            HDF5File::clear(_timeseries_filename, _comm);

        HDF5File file{_timeseries_filename, _comm, HDF5File::append};
        const auto offsets = _write_to(file);

        file.set_attribute("NSteps", this->_step_count+1, "VTKHDF/Steps");
        file.write(std::array{t}, {"VTKHDF/Steps", "Values"});
        file.write(std::vector{offsets.point_offset}, {"VTKHDF/Steps", "PointOffsets"});
        file.write(std::vector{std::array{offsets.cell_offset}}, {"VTKHDF/Steps", "CellOffsets"});
        file.write(std::vector{std::array{offsets.connectivity_offset}}, {"VTKHDF/Steps", "ConnectivityIdOffsets"});

        if (this->_step_count > 0 && _transient_opts.static_grid) {
            file.write(std::vector{_get_last_step_data(file, {"", "PartOffsets"})}, {"VTKHDF/Steps", "PartOffsets"});
        } else {
            file.write(std::vector{this->_step_count*Parallel::size(_comm)}, {"VTKHDF/Steps", "PartOffsets"});
        }

        return _timeseries_filename;
    }

    TimeSeriesOffsets _write_to(HDF5File& file) const {
        file.set_attribute("Version", std::array<std::size_t, 2>{(is_transient ? 2 : 1), 0}, "VTKHDF");
        file.set_attribute("Type", "UnstructuredGrid", "VTKHDF");

        TimeSeriesOffsets offsets;
        const auto context = IOContext::from(this->grid(), _comm, root_rank);
        _write_num_cells_and_points(file, context);
        offsets.point_offset = _write_coordinates(file, context);
        offsets.connectivity_offset = _write_connectivity(file, context);
        offsets.cell_offset = _write_types(file, context);
        _write_offsets(file, context);
        _write_meta_data(file);
        _write_point_fields(file, context);
        _write_cell_fields(file, context);

        return offsets;
    }

    void _write_num_cells_and_points(HDF5File& file, const IOContext& context) const {
        _write_values(file, {"VTKHDF", "NumberOfPoints"}, std::vector{number_of_points(this->grid())}, context);
        _write_values(file, {"VTKHDF", "NumberOfCells"}, std::vector{number_of_cells(this->grid())}, context);
    }

    std::size_t _write_coordinates(HDF5File& file, const IOContext& context) const {
        if constexpr (is_transient) {
            if (this->_step_count > 0 && _transient_opts.static_grid)
                return _get_last_step_data(file, {"", "PointOffsets"});
        }
        const auto coords_field = VTK::make_coordinates_field<CT>(this->grid(), false);
        std::vector<std::array<CT, vtk_space_dim>> coords(number_of_points(this->grid()));
        coords_field->export_to(coords);
        const auto offset = _get_current_offset(file, {"VTKHDF", "Points"});
        _write_point_field(file, {"VTKHDF", "Points"}, coords, context);
        return offset;
    }

    std::size_t _write_connectivity(HDF5File& file, const IOContext& context) const {
        if constexpr (is_transient) {
            if (this->_step_count > 0 && _transient_opts.static_grid)
                return _get_last_step_data(file, {"", "ConnectivityIdOffsets"});
        }
        const auto point_id_map = make_point_id_map(this->grid());
        const auto connectivity_field = VTK::make_connectivity_field(this->grid(), point_id_map);
        const auto num_entries = connectivity_field->layout().number_of_entries();
        const auto my_num_ids = std::vector{static_cast<long>(num_entries)};
        std::vector<long> connectivity(num_entries);
        connectivity_field->export_to(connectivity);
        const auto offset = _get_current_offset(file, {"VTKHDF", "Connectivity"});
        _write_values(file, {"VTKHDF", "Connectivity"}, connectivity, context);
        _write_values(file, {"VTKHDF", "NumberOfConnectivityIds"}, my_num_ids, context);
        return offset;
    }

    std::size_t _write_types(HDF5File& file, const IOContext& context) const {
        if constexpr (is_transient) {
            if (this->_step_count > 0 && _transient_opts.static_grid)
                return _get_last_step_data(file, {"", "CellOffsets"});
        }
        const auto types_field = VTK::make_cell_types_field(this->grid());
        std::vector<std::uint8_t> types(types_field->layout().number_of_entries());
        types_field->export_to(types);
        const auto offset = _get_current_offset(file, {"VTKHDF", "Types"});
        _write_values(file, {"VTKHDF", "Types"}, types, context);
        return offset;
    }

    std::size_t _write_offsets(HDF5File& file, const IOContext& context) const {
        if constexpr (is_transient) {
            if (this->_step_count > 0 && _transient_opts.static_grid)
                return _get_last_step_data(file, {"", "CellOffsets"});
        }
        const auto offsets_field = VTK::make_offsets_field(this->grid());
        const auto num_offset_entries = offsets_field->layout().number_of_entries() + 1;
        std::vector<long> offsets(num_offset_entries);
        offsets_field->export_to(std::ranges::subrange(std::next(offsets.begin()), offsets.end()));
        offsets[0] = long{0};
        const auto offset = _get_current_offset(file, {"VTKHDF", "Offsets"});
        _write_values(file, {"VTKHDF", "Offsets"}, offsets, context);
        return offset;
    }

    void _write_meta_data(HDF5File& file) const {
        std::ranges::for_each(this->_meta_data_field_names(), [&] (const std::string& name) {
            if constexpr (is_transient) {
                if (this->_step_count > 0 && _transient_opts.static_meta_data) {
                    file.write(std::array{0}, {"VTKHDF/Steps/FieldDataOffsets", name});
                    return;
                } else {
                    file.write(std::array{this->_step_count}, {"VTKHDF/Steps/FieldDataOffsets", name});
                }
            }

            auto field_ptr = this->_get_meta_data_field_ptr(name);
            _visit_field_values(*field_ptr, [&] <typename T> (T&& values) {
                file.write(std::array{std::forward<T>(values)}, {"VTKHDF/FieldData", name});
            });
        });
    }

    void _write_point_fields(HDF5File& file, const IOContext& context) const {
        std::ranges::for_each(this->_point_field_names(), [&] (const std::string& name) {
            if constexpr (is_transient)
                _write_step_offset(
                    file,
                    _get_current_offset(file, {"VTKHDF/PointData", name}),
                    {"VTKHDF/Steps/PointDataOffsets", name}
                );
            auto field_ptr = VTK::make_vtk_field(this->_get_point_field_ptr(name));
            _write_point_field(file, {"VTKHDF/PointData", name}, *field_ptr, context);
        });
    }

    void _write_cell_fields(HDF5File& file, const IOContext& context) const {
        std::ranges::for_each(this->_cell_field_names(), [&] (const std::string& name) {
            if constexpr (is_transient)
                _write_step_offset(
                    file,
                    _get_current_offset(file, {"VTKHDF/CellData", name}),
                    {"VTKHDF/Steps/CellDataOffsets", name}
                );
            auto field_ptr = VTK::make_vtk_field(this->_get_cell_field_ptr(name));
            _write_cell_field(file, {"VTKHDF/CellData", name}, *field_ptr, context);
        });
    }

    template<Concepts::Scalar T>
    void _write_values(HDF5File& file,
                       const DataSetPath& path,
                       const std::vector<T>& values,
                       const IOContext& context) const {
        if (context.is_parallel) {
            const auto num_values = values.size();
            const auto total_num_values = Parallel::sum(_comm, num_values, root_rank);
            const auto my_total_num_values = Parallel::broadcast(_comm, total_num_values, root_rank);
            const auto my_offset = _accumulate_rank_offset(num_values);
            DataSetSlice slice{
                .size = std::vector{my_total_num_values},
                .offset = std::vector{my_offset},
                .count = std::vector{num_values}
            };
            file.write(values, path, slice);
        } else {
            file.write(values, path);
        }
    }

    void _write_point_field(HDF5File& file,
                            const DataSetPath& path,
                            const Field& field,
                            const IOContext& context) const {
        _visit_field_values(field, [&] <typename T> (const T& values) {
            _write_field<true>(file, path, values, context);
        });
    }

    void _write_cell_field(HDF5File& file,
                           const DataSetPath& path,
                           const Field& field,
                           const IOContext& context) const {
        _visit_field_values(field, [&] <typename T> (const T& values) {
            _write_field<false>(file, path, values, context);
        });
    }

    template<typename FieldValues>
    void _write_point_field(HDF5File& file,
                            const DataSetPath& path,
                            const FieldValues& values,
                            const IOContext& context) const {
        _write_field<true>(file, path, values, context);
    }

    template<typename FieldValues>
    void _write_cell_field(HDF5File& file,
                           const DataSetPath& path,
                           const FieldValues& values,
                           const IOContext& context) const {
        _write_field<false>(file, path, values, context);
    }

    template<bool is_point_field, typename FieldValues>
    void _write_field(HDF5File& file,
                      const DataSetPath& path,
                      const FieldValues& values,
                      const IOContext& context) const {
        if (context.is_parallel) {
            const auto layout = get_md_layout(values);
            std::vector<std::size_t> count(layout.dimension());
            layout.export_to(count);

            std::vector<std::size_t> size = count;
            std::vector<std::size_t> offset(layout.dimension(), 0);
            offset.at(0) = is_point_field ? context.my_point_offset : context.my_cell_offset;
            size.at(0) = is_point_field ? context.num_points_total : context.num_cells_total;

            file.write(values, path, {
                .size = size,
                .offset = offset,
                .count = count
            });
        } else {
            file.write(values, path);
        }
    }

    template<typename Visitor>
    void _visit_field_values(const Field& field, const Visitor& visitor) const {
        field.precision().visit([&] <typename T> (const Precision<T>&) {
            const auto layout = field.layout();
            if (layout.dimension() == 1) {
                std::vector<T> data(layout.extent(0));
                field.export_to(data);
                visitor(data);
            } else if (layout.dimension() == 2) {
                std::vector<Vector<T>> data(layout.extent(0));
                field.export_to(data);
                visitor(data);
            } else if (layout.dimension() == 3) {
                std::vector<Tensor<T>> data(layout.extent(0));
                field.export_to(data);
                visitor(data);
            } else {
                throw NotImplemented("Support for fields with dimension > 3 or < 1");
            }
        });
    }

    std::size_t _accumulate_rank_offset(std::size_t my_size) const {
        auto all_offsets = Parallel::gather(_comm, my_size, root_rank);
        if (Parallel::rank(_comm) == root_rank) {
            std::partial_sum(all_offsets.begin(), std::prev(all_offsets.end()), all_offsets.begin());
            std::copy(std::next(all_offsets.rbegin()), all_offsets.rend(), all_offsets.rbegin());
            all_offsets[0] = 0;
        }
        const auto my_offset = Parallel::scatter(_comm, all_offsets, root_rank);
        if (my_offset.size() != 1)
            throw ValueError("Unexpected scatter result");
        return my_offset[0];
    }

    void _write_step_offset(HDF5File& file,
                            const std::size_t offset,
                            const DataSetPath& path) const {
        file.write(std::array{offset}, path);
    }

    std::size_t _get_current_offset(HDF5File& file, const DataSetPath& path) const {
        const auto cur_dimensions = file.get_dimensions(path);
        return cur_dimensions ? (*cur_dimensions)[0] : 0;
    }

    std::size_t _get_last_step_data(const HDF5File& file, const DataSetPath& sub_path) const {
        if (this->_step_count == 0)
            throw ValueError("Last step data can only be read after at least one write");
        const DataSetPath path{"VTKHDF/Steps/" + sub_path.group_path, sub_path.dataset_name};

        auto step_dimensions = file.get_dimensions(path).value();
        std::ranges::fill(step_dimensions, std::size_t{1});

        auto access_offset = step_dimensions;
        std::ranges::fill(access_offset, std::size_t{0});
        access_offset.at(0) = this->_step_count - 1;

        return file.template read<std::size_t>(path, {
                .size = std::vector<std::size_t>{},  // TODO: Slice should be a structure without size
                .offset = access_offset,
                .count = step_dimensions
            }
        );
    }

    Communicator _comm;
    std::string _timeseries_filename = "";
    VTK::HDFTransientOptions _transient_opts;
};

template<Concepts::UnstructuredGrid G, Concepts::Communicator C = NullCommunicator>
class VTKHDFUnstructuredGridWriter : public VTKHDFUnstructuredGridWriterImpl<false, G, C> {
    using ParentType = VTKHDFUnstructuredGridWriterImpl<false, G, C>;
 public:
    using ParentType::ParentType;
};

template<Concepts::UnstructuredGrid G, Concepts::Communicator C = NullCommunicator>
class VTKHDFUnstructuredTimeSeriesWriter : public VTKHDFUnstructuredGridWriterImpl<true, G, C> {
    using ParentType = VTKHDFUnstructuredGridWriterImpl<true, G, C>;
 public:
    using ParentType::ParentType;
};

template<Concepts::UnstructuredGrid G>
VTKHDFUnstructuredGridWriter(const G&) -> VTKHDFUnstructuredGridWriter<G, NullCommunicator>;
template<Concepts::UnstructuredGrid G, Concepts::Communicator C>
VTKHDFUnstructuredGridWriter(const G&, const C&) -> VTKHDFUnstructuredGridWriter<G, C>;

template<Concepts::UnstructuredGrid G>
VTKHDFUnstructuredTimeSeriesWriter(const G&, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFUnstructuredTimeSeriesWriter<G, NullCommunicator>;
template<Concepts::UnstructuredGrid G, Concepts::Communicator C>
VTKHDFUnstructuredTimeSeriesWriter(const G&, const C&, std::string, VTK::HDFTransientOptions = {}) -> VTKHDFUnstructuredTimeSeriesWriter<G, C>;

}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_UNSTRUCTURED_GRID_WRITER_HPP_
