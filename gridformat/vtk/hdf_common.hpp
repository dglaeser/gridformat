// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for writing VTK HDF files.
 */
#ifndef GRIDFORMAT_VTK_HDF_COMMON_HPP_
#define GRIDFORMAT_VTK_HDF_COMMON_HPP_
#if GRIDFORMAT_HAVE_HIGH_FIVE

#include <type_traits>
#include <algorithm>
#include <concepts>
#include <ranges>
#include <cstdint>

#ifdef GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif  // GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS

#include <highfive/H5Easy.hpp>
#include <highfive/H5File.hpp>

#ifdef GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS
#pragma GCC diagnostic pop
#endif  // GRIDFORMAT_DISABLE_HIGHFIVE_WARNINGS

#include <gridformat/common/logging.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/buffer_field.hpp>
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/grid.hpp>


namespace GridFormat {

namespace VTK {

struct HDFTransientOptions {
    bool static_grid = false; //!< Set to true if the metadata is same for all time steps (will only be written once)
    bool static_meta_data = false; //!< Set to true the grid is the same for all time steps (will only be written once)
};

}  // namespace VTK


namespace VTKHDF {

struct DataSetSlice {
    std::vector<std::size_t> total_size = {};
    std::vector<std::size_t> offset;
    std::vector<std::size_t> count;
};

struct DataSetPath {
    std::string group_path;
    std::string dataset_name;
};

#ifndef DOXYGEN
std::pair<std::string, std::string> split_name(const std::string& in) {
    if (in.ends_with('/'))
        return {in, ""};

    const auto split_pos = in.find_last_of("/");
    if (split_pos == std::string::npos )
        throw ValueError("Could not split name from given path: " + in);
    return {
        split_pos > 0 ? in.substr(0, split_pos) : "/",
        in.substr(split_pos + 1)
    };
}

struct IOContext {
    const int my_rank;
    const int num_ranks;
    const bool is_parallel;
    const std::vector<std::size_t> rank_cells;
    const std::vector<std::size_t> rank_points;
    const std::size_t num_cells_total;
    const std::size_t num_points_total;
    const std::size_t my_cell_offset;
    const std::size_t my_point_offset;

    IOContext(int _my_rank,
              int _num_ranks,
              std::vector<std::size_t> _rank_cells,
              std::vector<std::size_t> _rank_points)
    : my_rank{_my_rank}
    , num_ranks{_num_ranks}
    , is_parallel{_num_ranks > 1}
    , rank_cells{std::move(_rank_cells)}
    , rank_points{std::move(_rank_points)}
    , num_cells_total{_accumulate(rank_cells)}
    , num_points_total{_accumulate(rank_points)}
    , my_cell_offset{_accumulate_rank_offset(rank_cells)}
    , my_point_offset{_accumulate_rank_offset(rank_points)} {
        if (my_rank >= num_ranks)
            throw ValueError(as_error("Given rank is not within communicator size"));
        if (num_ranks != static_cast<int>(rank_cells.size()))
            throw ValueError(as_error("Cells vector does not match communicator size"));
        if (num_ranks != static_cast<int>(rank_points.size()))
            throw ValueError(as_error("Points vector does not match communicator size"));
    }

    template<Concepts::Grid Grid, Concepts::Communicator Communicator>
    static IOContext from(const Grid& grid, const Communicator& comm, int root_rank = 0) {
        const int size = Parallel::size(comm);
        const int rank = Parallel::rank(comm);
        const std::size_t num_points = number_of_points(grid);
        const std::size_t num_cells = number_of_cells(grid);
        if (size == 1)
            return IOContext{rank, size, std::vector{num_cells}, std::vector{num_points}};

        const auto all_num_points = Parallel::gather(comm, num_points, root_rank);
        const auto all_num_cells = Parallel::gather(comm, num_cells, root_rank);
        const auto my_all_num_points = Parallel::broadcast(comm, all_num_points, root_rank);
        const auto my_all_num_cells = Parallel::broadcast(comm, all_num_cells, root_rank);
        return IOContext{rank, size, my_all_num_cells, my_all_num_points};
    }

 private:
    std::size_t _accumulate(const std::vector<std::size_t>& in) const {
        return std::accumulate(in.begin(), in.end(), std::size_t{0});
    }

    std::size_t _accumulate_rank_offset(const std::vector<std::size_t>& in) const {
        if (in.size() <= static_cast<std::size_t>(my_rank))
            throw ValueError("Rank-vector length must be equal to number of ranks");
        return std::accumulate(in.begin(), std::next(in.begin(), my_rank), std::size_t{0});
    }
};

// Custom string data type using ascii encoding: VTKHDF uses ascii, but HighFive uses UTF-8.
struct AsciiString : public HighFive::DataType {
    explicit AsciiString(std::size_t n) {
        _hid = H5Tcopy(H5T_C_S1);
        if (H5Tset_size(_hid, n) < 0) {
            HighFive::HDF5ErrMapper::ToException<HighFive::DataTypeException>(
                "Unable to define datatype size to " + std::to_string(n)
            );
        }
        // define encoding to ASCII
        H5Tset_cset(_hid, H5T_CSET_ASCII);
        H5Tset_strpad(_hid, H5T_STR_SPACEPAD);
    }

    template<std::size_t N>
    static AsciiString from(const char (&input)[N]) {
        if (input[N-1] == '\0')
            return AsciiString{N-1};
        return AsciiString{N};
    }

    static AsciiString from(const std::string& n) {
        return AsciiString{n.size()};
    }
};

namespace Detail {
    HighFive::DataTransferProps parallel_transfer_props() {
        HighFive::DataTransferProps xfer_props;
#if GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        xfer_props.add(HighFive::UseCollectiveIO{});
#else
        throw NotImplemented("Parallel HighFive required for parallel I/O");
#endif
        return xfer_props;
    }

    template<Concepts::Communicator Communicator>
    auto parallel_file_access_props([[maybe_unused]] const Communicator& communicator) {
        HighFive::FileAccessProps fapl;
#if GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        if constexpr (!std::is_same_v<Communicator, NullCommunicator>) {
            fapl.add(HighFive::MPIOFileAccess{communicator, MPI_INFO_NULL});
            fapl.add(HighFive::MPIOCollectiveMetadata{});
        } else {
            throw TypeError("Cannot establish parallel I/O with null communicator");
        }
#else
        throw NotImplemented("Parallel HighFive required for parallel I/O");
#endif
        return fapl;
    }

    void check_successful_collective_io([[maybe_unused]] const HighFive::DataTransferProps& xfer_props) {
#if GRIDFORMAT_HAVE_PARALLEL_HIGH_FIVE
        auto mnccp = HighFive::MpioNoCollectiveCause(xfer_props);
        if (mnccp.getLocalCause() || mnccp.getGlobalCause())
            log_warning(
                std::string{"The operation was successful, but couldn't use collective MPI-IO. "}
                + "Local cause: " + std::to_string(mnccp.getLocalCause()) + " "
                + "Global cause:" + std::to_string(mnccp.getGlobalCause()) + "\n"
            );
#else
        throw NotImplemented("Parallel HighFive required for parallel I/O");
#endif
    }

}  // namespace Detail


template<Concepts::Communicator Communicator>
class HDF5File {
 public:
    enum Mode { overwrite, append, read_only };

    HDF5File(const std::string& filename, const Communicator& comm, Mode mode)
    : _comm{comm}
    , _mode{mode}
    , _file{_open(filename)}
    {}

    static void clear(const std::string& filename, const Communicator& comm) {
        if (Parallel::rank(comm) == 0)  // clear file by open it in overwrite mode
            HighFive::File{filename, HighFive::File::Overwrite};
        Parallel::barrier(comm);
    }

    template<typename Attribute>
    void set_attribute(const std::string& name,
                       const Attribute& attribute,
                       const std::string& group_name) {
        _check_writable();
        _clear_attribute(name, group_name);
        _get_group(group_name).createAttribute(name, attribute);
    }

    template<std::size_t N>
    void set_attribute(const std::string& name,
                       const char (&attribute)[N],
                       const std::string& group_name) {
        _check_writable();
        _clear_attribute(name, group_name);
        auto type_attr = _get_group(group_name).createAttribute(
            name,
            HighFive::DataSpace{1},
            AsciiString::from(attribute)
        );
        type_attr.write(attribute);
    }

    template<typename Values>
    std::size_t write(const Values& values, const DataSetPath& path) {
        _check_writable();
        const auto space = HighFive::DataSpace::From(values);
        auto group = _get_group(path.group_path);
        auto [offset, dataset] = _prepare_dataset<FieldScalar<Values>>(group, path.dataset_name, space);

        std::vector<std::size_t> ds_size = space.getDimensions();
        std::vector<std::size_t> ds_offset(ds_size.size(), 0);
        ds_offset.at(0) += offset;

        // do the actual write only on rank 0 to avoid clashes
        if (Parallel::rank(_comm) != 0)
            std::ranges::fill(ds_size, 0);

        _write_to(dataset, values, {.offset = ds_offset, .count = ds_size});
        _file.flush();
        return space.getDimensions()[0];
    }

    template<typename Values>
    std::size_t write(const Values& values, const DataSetPath& path, DataSetSlice slice) {
        _check_writable();
        const auto space = HighFive::DataSpace(slice.total_size);
        auto group = _get_group(path.group_path);
        auto [offset, dataset] = _prepare_dataset<FieldScalar<Values>>(group, path.dataset_name, space);

        slice.offset[0] += offset;
        if (Parallel::size(_comm) > 1) {
            const auto props = Detail::parallel_transfer_props();
            _write_to(dataset, values, slice, props);
            Detail::check_successful_collective_io(props);
        } else {
            _write_to(dataset, values, slice);
        }
        _file.flush();
        return space.getDimensions()[0];
    }

    template<Concepts::Scalar T>
    T read(const DataSetPath& path, const DataSetSlice& slice) const {
        T result;
        if (_file.exist(path.group_path))
            if (_file.getGroup(path.group_path).exist(path.dataset_name)) {
                _file.getGroup(path.group_path).getDataSet(path.dataset_name).select(
                    slice.offset, slice.count
                ).read(result);
                return result;
            }
        throw ValueError("Given group or dataset does not exist");
    }

    template<Concepts::ResizableMDRange T>
    T read_dataset_to(const std::string& path, const std::optional<DataSetSlice>& slice = {}) const {
        return visit_dataset(path, [&] <typename F> (BufferField<F>&& field) {
            return field.template export_to<T>();
        }, slice);
    }

    template<std::invocable<BufferField<int>&&> Visitor>
    decltype(auto) visit_dataset(const std::string& path,
                                 Visitor&& visitor,
                                 const std::optional<DataSetSlice>& slice = {}) const {
        auto [group, name] = split_name(path);
        if (!has_dataset(group, name))
            throw ValueError("Given data set '" + path + "' does not exist.");
        if (slice)
            return _visit_data(
                std::forward<Visitor>(visitor),
                _file.getGroup(group).getDataSet(name).select(slice->offset, slice->count)
            );
        return _visit_data(std::forward<Visitor>(visitor), _file.getGroup(group).getDataSet(name));
    }

    template<Concepts::ResizableMDRange T>
    T read_attribute_to(const std::string& path) const {
        return visit_attribute(path, [&] <typename F> (BufferField<F>&& field) {
            return field.template export_to<T>();
        });
    }

    template<std::invocable<BufferField<int>&&> Visitor>
    decltype(auto) visit_attribute(const std::string& path, Visitor&& visitor) const {
        auto [group, name] = split_name(path);
        if (!has_attribute(group, name))
            throw ValueError("Given attribute '" + path + "' does not exist.");
        return _visit_data(std::forward<Visitor>(visitor), _file.getGroup(group).getAttribute(name));
    }

    std::optional<std::vector<std::size_t>> get_dimensions(const DataSetPath& path) const {
        if (has_dataset(path.group_path, path.dataset_name))
            return _file.getGroup(path.group_path).getDataSet(path.dataset_name).getDimensions();
        return {};
    }

    std::ranges::range auto dataset_names_in(const std::string& group) const {
        return _file.getGroup(group).listObjectNames()
            | std::views::filter([&, group=group] (const auto& name) {
                return _file.getGroup(group).getObjectType(name) == HighFive::ObjectType::Dataset;
            });
    }

    bool exists(const std::string& path) const {
        return _file.exist(path);
    }

    bool has_dataset(const std::string& path, const std::string& name) const {
        if (_file.exist(path))
            if (_file.getGroup(path).exist(name))
                return _file.getGroup(path).getObjectType(name) == HighFive::ObjectType::Dataset;
        return false;
    }

    bool has_attribute(const std::string& path, const std::string& name) const {
        if (_file.exist(path)) {
            const auto [group, object] = split_name(path);
            if (_file.getGroup(group).getObjectType(object) == HighFive::ObjectType::Dataset)
                return _file.getGroup(group).getDataSet(object).hasAttribute(name);
            else if (_file.getGroup(group).getObjectType(object) == HighFive::ObjectType::Group)
                return _file.getGroup(group).getGroup(object).hasAttribute(name);
            throw NotImplemented("Unsupported object type at '" + path + "'.");
        }
        return false;
    }

 private:
    void _check_writable() const {
        if (_mode == read_only)
            throw InvalidState("Cannot modify hdf-file opened in read-only mode");
    }

    HighFive::File _open(const std::string& filename) const {
        auto open_mode = _mode == read_only ? HighFive::File::ReadOnly
                                            : (_mode == overwrite ? HighFive::File::Overwrite
                                                                  : HighFive::File::ReadWrite);
        if (Parallel::size(_comm) > 1)
            return HighFive::File{filename, open_mode, Detail::parallel_file_access_props(_comm)};
        else
            return HighFive::File{filename, open_mode};
    }

    template<typename T>
    auto _prepare_dataset(HighFive::Group& group,
                          const std::string& name,
                          const HighFive::DataSpace& space) {
        if (_mode == overwrite)
            return std::make_pair(std::size_t{0}, group.createDataSet(name, space, HighFive::create_datatype<T>()));
        else if (_mode == append) {
            if (group.exist(name)) {
                auto dataset = group.getDataSet(name);
                auto out_dimensions = dataset.getDimensions();
                const auto in_dimensions = space.getDimensions();

                if (out_dimensions.size() < 1 || in_dimensions.size() < 1)
                    throw ValueError("Cannot extend scalar datasets");
                if (
                    !std::ranges::equal(
                        std::ranges::subrange(out_dimensions.begin() + 1, out_dimensions.end()),
                        std::ranges::subrange(in_dimensions.begin() + 1, in_dimensions.end())
                    )
                )
                    throw ValueError("Dataset extension requires the sub-dimensions to be equal");

                const std::size_t offset = out_dimensions[0];
                out_dimensions[0] += in_dimensions[0];
                dataset.resize(out_dimensions);
                return std::make_pair(offset, std::move(dataset));
            } else {
                const auto init_dimensions = space.getDimensions();
                if (init_dimensions.size() < 1)
                    throw ValueError("Scalars cannot be written in appended mode. Wrap them in std::array{scalar}");

                const auto chunk_dimensions = std::vector<hsize_t>{init_dimensions.begin(), init_dimensions.end()};
                const auto max_dimensions = [&] () {
                    auto tmp = init_dimensions;
                    tmp[0] *= HighFive::DataSpace::UNLIMITED;
                    return tmp;
                } ();

                HighFive::DataSpace out_space(init_dimensions, max_dimensions);
                HighFive::DataSetCreateProps props;
                props.add(HighFive::Chunking(chunk_dimensions));
                return std::make_pair(
                    std::size_t{0},
                    group.createDataSet(name, out_space, HighFive::create_datatype<T>(), props)
                );
            }
        } else {
            throw NotImplemented("Unknown file mode");
        }
    }

    template<typename Values>
    void _write_to(HighFive::DataSet& dataset,
                   const Values& values,
                   const DataSetSlice& slice,
                   const std::optional<HighFive::DataTransferProps> props = {}) {
        props ? dataset.select(slice.offset, slice.count).write(values, *props)
              : dataset.select(slice.offset, slice.count).write(values);
    }

    template<typename Visitor, typename Source>
    decltype(auto) _visit_data(Visitor&& visitor, const Source& source) const {
        const auto datatype = source.getDataType();
        if (datatype.getClass() == HighFive::DataTypeClass::Float && datatype.getSize() == 8)
            return visitor(_read_buffer_field<double>(source));
        else if (datatype.getClass() == HighFive::DataTypeClass::Float && datatype.getSize() == 4)
            return visitor(_read_buffer_field<float>(source));
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 1)
            return visitor(_read_buffer_field<std::int_least8_t>(source));
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 2)
            return visitor(_read_buffer_field<std::int_least16_t>(source));
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 4)
            return visitor(_read_buffer_field<std::int_least32_t>(source));
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 8)
            return visitor(_read_buffer_field<std::int_least64_t>(source));
        else if (datatype.isVariableStr())
            return visitor(_read_buffer_field<char>(source));
        else if (datatype.isFixedLenStr()) {
            static constexpr std::size_t N = 100;
            HighFive::FixedLenStringArray<N> pre_out;
            source.read(pre_out);
            if (pre_out.size() > 1)
                throw SizeError("Unexpected string array size");

            std::string out;
            std::ranges::copy(
                pre_out.getString(0) | std::views::take_while([] (const char c) { return c != '\0'; }),
                std::back_inserter(out)
            );
            MDLayout layout{{out.size()}};
            return visitor(BufferField{std::move(out), std::move(layout)});
        } else
            throw ValueError("Unsupported data type '" + datatype.string() + "'");
    }

    template<typename T, typename Source>
    auto _read_buffer_field(const Source& source) const {
        MDLayout layout{source.getMemSpace().getDimensions()};
        std::vector<T> out(layout.number_of_entries());
        source.read(out.data());
        return BufferField(std::move(out), std::move(layout));
    }

    HighFive::Group _get_group(const std::string& group_name) {
        return _file.exist(group_name) ? _file.getGroup(group_name) : _file.createGroup(group_name);
    }

    void _clear_attribute(const std::string& name, const std::string& group) {
        if (_get_group(group).hasAttribute(name))
            _get_group(group).deleteAttribute(name);
    }

    Communicator _comm;
    Mode _mode;
    HighFive::File _file;
};

}  // namespace VTKHDF
}  // namespace GridFormat

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_VTK_HDF_COMMON_HPP_
