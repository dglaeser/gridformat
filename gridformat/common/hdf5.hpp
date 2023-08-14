// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT
/*!
 * \file
 * \ingroup Common
 * \brief Helpers for I/O from HDF5 files.
 */
#ifndef GRIDFORMAT_COMMON_HDF5_HPP_
#define GRIDFORMAT_COMMON_HDF5_HPP_
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

#include <gridformat/common/field.hpp>
#include <gridformat/common/logging.hpp>
#include <gridformat/common/concepts.hpp>
#include <gridformat/common/md_layout.hpp>
#include <gridformat/common/buffer_field.hpp>
#include <gridformat/common/precision.hpp>

#include <gridformat/parallel/communication.hpp>
#include <gridformat/grid/grid.hpp>


namespace GridFormat::HDF5 {

#ifndef DOXYGEN
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

    std::pair<std::string, std::string> split_group(const std::string& in) {
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

}  // namespace Detail
#endif  // DOXYGEN

//! Represents a dataset slice
struct Slice {
    std::vector<std::size_t> offset;
    std::vector<std::size_t> count;
    std::optional<std::vector<std::size_t>> total_size = {};
};

//! Custom string data type using ascii encoding.
//! HighFive uses UTF-8, but VTKHDF, for instance, uses ascii.
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

/*!
 * \ingroup Common
 * \brief Helper class for I/O from HDF5 files.
 * \note Currently, strings are ascii encoded, in contrast to the standard utf-8
 *       that HighFive uses. Once necessary, we'll make the encoding settable.
 */
template<Concepts::Communicator Communicator = NullCommunicator>
class File {
 public:
    enum Mode {
        overwrite,  //!< remove all content before writing
        append,     //!< append given data to existing datasets
        read_only   //!< only read data
    };

    File(const std::string& filename, Mode mode = Mode::read_only)
        requires(std::same_as<Communicator, NullCommunicator>)
    : File(filename, NullCommunicator{}, mode)
    {}

    File(const std::string& filename, const Communicator& comm, Mode mode = Mode::read_only)
    : _comm{comm}
    , _mode{mode}
    , _file{_open(filename)}
    {}

    //! Clear the contents of the file with the given name
    static void clear(const std::string& filename, const Communicator& comm) {
        if (Parallel::rank(comm) == 0)  // clear file by open it in overwrite mode
            HighFive::File{filename, HighFive::File::Overwrite};
        Parallel::barrier(comm);
    }

    //! Write the given values to the attribute with the given name into the given group
    template<typename Values>
    void write_attribute(const Values& values, const std::string& path) {
        _check_writable();
        const auto [group, name] = Detail::split_group(path);
        _clear_attribute(name, group);
        _get_group(group).createAttribute(name, values);
    }

    //! Write the given characters to the attribute with the given name into the given group
    template<std::size_t N>
    void write_attribute(const char (&values)[N], const std::string& path) {
        _check_writable();
        const auto [group, name] = Detail::split_group(path);
        _clear_attribute(name, group);
        auto type_attr = _get_group(group).createAttribute(
            name,
            HighFive::DataSpace{1},
            AsciiString::from(values)
        );
        type_attr.write(values);
    }

    //! Write the given values into the dataset with the given path
    template<std::ranges::range Values>
    void write(const Values& values,
               const std::string& path,
               const std::optional<Slice>& slice = {}) {
        _check_writable();
        const auto [group_name, ds_name] = Detail::split_group(path);
        const auto space = slice ? HighFive::DataSpace{slice->total_size.value()}
                                 : HighFive::DataSpace::From(values);
        auto group = _get_group(group_name);
        auto [offset, dataset] = _prepare_dataset<FieldScalar<Values>>(group, ds_name, space);
        Slice _slice{
            .offset = slice ? slice->offset : std::vector<std::size_t>(space.getNumberDimensions(), 0),
            .count = slice ? slice->count : space.getDimensions()
        };
        _slice.offset.at(0) += offset;

        if (Parallel::size(_comm) > 1) {
            if (slice) {  // collective I/O
                const auto props = Detail::parallel_transfer_props();
                _write_to(dataset, values, _slice, props);
                Detail::check_successful_collective_io(props);
            } else if (Parallel::rank(_comm) == 0) {  // write only on rank 0 to avoid clashes
                _write_to(dataset, values, _slice);
            }
        } else {
            _write_to(dataset, values, _slice);
        }
        _file.flush();
    }

    //! Write a field into the dataset with the given path
    void write(const Field& field,
               const std::string& path,
               const std::optional<Slice>& slice = {}) {
        _check_writable();
        const auto layout = field.layout();
        const HighFive::DataSpace space{[&] () {
            if (slice)
                return slice->total_size.value();
            std::vector<std::size_t> dims(layout.dimension());
            layout.export_to(dims);
            return dims;
        } ()};

        const auto [group_name, ds_name] = Detail::split_group(path);
        auto group = _get_group(group_name);
        field.precision().visit([&] <typename T> (const Precision<T>&) {
            auto [offset, dataset] = _prepare_dataset<T>(group, ds_name, space);
            Slice _slice{
                .offset = slice ? slice->offset : std::vector<std::size_t>(space.getNumberDimensions(), 0),
                .count = slice ? slice->count : space.getDimensions()
            };
            _slice.offset.at(0) += offset;

            const auto serialization = field.serialized();
            const std::span<const T> span = serialization.template as_span_of<T>();

            if (Parallel::size(_comm) > 1) {
                if (slice) {  // collective I/O
                    const auto props = Detail::parallel_transfer_props();
                    _write_to(dataset, span.data(), _slice, props);
                    Detail::check_successful_collective_io(props);
                } else if (Parallel::rank(_comm) == 0) {  // write only on rank 0 to avoid clashes
                    _write_to(dataset, span.data(), _slice);
                }
            } else {
                _write_to(dataset, span.data(), _slice);
            }
            _file.flush();
        });
    }

    //! Read dataset values into an instance of the given T
    template<typename T>
        requires(Concepts::ResizableMDRange<T> or Concepts::Scalar<T>)
    T read_dataset_to(const std::string& path,
                      const std::optional<Slice>& slice = {}) const {
        return visit_dataset(path, [&] <typename F> (BufferField<F>&& field) {
            return field.template export_to<T>();
        }, slice);
    }

    //! Visit the dataset field
    template<std::invocable<BufferField<int>&&> Visitor>
    decltype(auto) visit_dataset(const std::string& path,
                                 Visitor&& visitor,
                                 const std::optional<Slice>& slice = {}) const {
        if (!has_dataset_at(path))
            throw ValueError("Given data set '" + path + "' does not exist.");

        auto [group, name] = Detail::split_group(path);
        if (slice)
            return _visit_data(
                std::forward<Visitor>(visitor),
                _file.getGroup(group).getDataSet(name).select(slice->offset, slice->count)
            );
        return _visit_data(std::forward<Visitor>(visitor), _file.getGroup(group).getDataSet(name));
    }

    //! Read attribute values into an instance of the given T
    template<typename T>
        requires(Concepts::ResizableMDRange<T> or Concepts::Scalar<T>)
    T read_attribute_to(const std::string& path) const {
        return visit_attribute(path, [&] <typename F> (BufferField<F>&& field) {
            return field.template export_to<T>();
        });
    }

    //! Visit the attribute field
    template<std::invocable<BufferField<int>&&> Visitor>
    decltype(auto) visit_attribute(const std::string& path, Visitor&& visitor) const {
        if (!has_attribute_at(path))
            throw ValueError("Given attribute '" + path + "' does not exist.");

        auto [group, name] = Detail::split_group(path);
        return _visit_data(std::forward<Visitor>(visitor), _file.getGroup(group).getAttribute(name));
    }

    //! Get the dimensions of a dataset; returns null optional if it doesn't exist.
    std::optional<std::vector<std::size_t>> get_dimensions(const std::string& path) const {
        if (has_dataset_at(path)) {
            const auto [group, name] = Detail::split_group(path);
            return _file.getGroup(group).getDataSet(name).getDimensions();
        }
        return {};
    }

    //! Get the precision of a dataset; returns null optional if it doesn't exist.
    std::optional<DynamicPrecision> get_precision(const std::string& path) const {
        if (has_dataset_at(path)) {
            const auto [group, name] = Detail::split_group(path);
            return _to_precision(_file.getGroup(group).getDataSet(name).getDataType());
        }
        return {};
    }

    //! Return the names of all datasets in the given group
    std::ranges::range auto dataset_names_in(const std::string& group) const {
        return _file.getGroup(group).listObjectNames()
            | std::views::filter([&, group=group] (const auto& name) {
                return _file.getGroup(group).getObjectType(name) == HighFive::ObjectType::Dataset;
            });
    }

    //! Return true if the given path exists in the file
    bool exists(const std::string& path) const {
        return _file.exist(path);
    }

    //! Return true if a dataset exists at the given path
    bool has_dataset_at(const std::string& path) const {
        if (_file.exist(path)) {
            const auto [group, name] = Detail::split_group(path);
            if (_file.getGroup(group).exist(name))
                return _file.getGroup(group).getObjectType(name) == HighFive::ObjectType::Dataset;
        }
        return false;
    }

    //! Return true if an attribute exists at the given path
    bool has_attribute_at(const std::string& path) const {
        const auto [parent_path, attr_name] = Detail::split_group(path);
        if (_file.exist(parent_path)) {
            if (has_dataset_at(parent_path)) {
                const auto [group, dataset] = Detail::split_group(parent_path);
                return _file.getGroup(group).getDataSet(dataset).hasAttribute(attr_name);
            } else {
                return _file.getGroup(parent_path).hasAttribute(attr_name);
            }
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
                   const Slice& slice,
                   const std::optional<HighFive::DataTransferProps> props = {}) {
        props ? dataset.select(slice.offset, slice.count).write(values, *props)
              : dataset.select(slice.offset, slice.count).write(values);
    }

    template<Concepts::Scalar T>
    void _write_to(HighFive::DataSet& dataset,
                   const T* buffer,
                   const Slice& slice,
                   const std::optional<HighFive::DataTransferProps> props = {}) {
        props ? dataset.select(slice.offset, slice.count).write_raw(buffer, dataset.getDataType(), *props)
              : dataset.select(slice.offset, slice.count).write_raw(buffer, dataset.getDataType());
    }

    template<typename Visitor, typename Source>
    decltype(auto) _visit_data(Visitor&& visitor, const Source& source) const {
        const auto datatype = source.getDataType();
        if (datatype.isFixedLenStr()) {
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
        } else {
            return _to_precision(datatype).visit([&] <typename T> (const Precision<T>&) {
                return visitor(_read_buffer_field<T>(source));
            });
        }
    }

    DynamicPrecision _to_precision(const HighFive::DataType& datatype) const {
        if (datatype.getClass() == HighFive::DataTypeClass::Float && datatype.getSize() == 8)
            return float64;
        else if (datatype.getClass() == HighFive::DataTypeClass::Float && datatype.getSize() == 4)
            return float32;
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 1)
            return int8;
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 2)
            return int16;
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 4)
            return int32;
        else if (datatype.getClass() == HighFive::DataTypeClass::Integer && datatype.getSize() == 8)
            return int64;
        else if (datatype.isVariableStr())
            return Precision<char>{};
        else if (datatype.isFixedLenStr())
            return Precision<char>{};
        throw NotImplemented("Could not determine data set precision");
    }

    template<typename T, typename Source>
    auto _read_buffer_field(const Source& source) const {
        auto dims = source.getMemSpace().getDimensions();
        MDLayout layout = dims.empty() ? MDLayout{{1}} : MDLayout{std::move(dims)};
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

}  // namespace GridFormat::HDF5

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE
#endif  // GRIDFORMAT_COMMON_HDF5_HPP_
