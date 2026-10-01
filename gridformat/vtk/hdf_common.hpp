// SPDX-FileCopyrightText: 2022-2023 Dennis Gläser <dennis.glaeser@iws.uni-stuttgart.de>
// SPDX-License-Identifier: MIT

/*!
 * \file
 * \ingroup VTK
 * \brief Common functionality for writing VTK HDF files.
 */
#ifndef GRIDFORMAT_VTK_HDF_COMMON_HPP_
#define GRIDFORMAT_VTK_HDF_COMMON_HPP_

#include <vector>
#include <cstddef>
#include <numeric>
#include <string>
#include <exception>

#include <gridformat/common/hdf5.hpp>
#include <gridformat/common/field.hpp>
#include <gridformat/common/precision.hpp>
#include <gridformat/common/lazy_field.hpp>
#include <gridformat/common/string_conversion.hpp>
#include <gridformat/common/exceptions.hpp>
#include <gridformat/parallel/communication.hpp>

namespace GridFormat {

namespace VTK {

//! Options for transient vtk-hdf file formats
struct HDFTransientOptions {
    bool static_grid = false; //!< Set to true the grid is the same for all time steps (will only be written once)
    bool static_meta_data = true; //!< Set to true if the metadata is same for all time steps (will only be written once)
};

}  // namespace VTK


namespace VTKHDF {

//! Helper class to store processor offsets in parallel writes
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

//! Throw if the given name cannot be used as a vtk-hdf array name
inline void check_array_name(const std::string& name) {
    if (name.empty())
        throw ValueError("VTKHDF array names must not be empty");
    if (name.find_first_of("/.") != std::string::npos)
        throw ValueError("VTKHDF array names must not contain '/' or '.' (received '" + name + "')");
}

//! Return true if the given field holds a single string (i.e. is a flat array of characters).
//! Multi-dimensional character fields are not treated as strings, since the number of strings
//! per time step is not expressible in the offsets that transient files store.
inline bool is_string_field(const Field& field) {
    return field.precision().template is<char>() && field.layout().dimension() == 1;
}

//! Return the string stored in the given character field, stripped of trailing null terminators
inline std::string extract_string(const Field& field) {
    const auto serialization = field.serialized();
    const auto characters = serialization.template as_span_of<const char>();
    std::string result{characters.begin(), characters.end()};
    while (!result.empty() && result.back() == '\0')
        result.pop_back();
    return result;
}

#if GRIDFORMAT_HAVE_HIGH_FIVE

/*!
 * \ingroup VTKHDF
 * \brief Field implementation that draws values from an open HDF5 file upon request.
 */
template<typename C>
class DataSetField : public LazyField<const HDF5::File<C>&> {
    using ParentType = LazyField<const HDF5::File<C>&>;

 public:
    using ParentType::ParentType;

    explicit DataSetField(const HDF5::File<C>& file, std::string path)
    : ParentType{
        file,
        MDLayout{file.get_dimensions(path).value()},
        DynamicPrecision{file.get_precision(path).value()},
        [_p=std::move(path)] (const HDF5::File<C>& file) {
            return file.visit_dataset(_p, [&] <typename F> (F&& field) {
                return field.serialized();
            });
        }
    } {}
};

template<typename C>
DataSetField(const HDF5::File<C>&, std::string) -> DataSetField<C>;

template<typename C, typename CB>
DataSetField(const HDF5::File<C>&, MDLayout, DynamicPrecision, CB&&) -> DataSetField<C>;

//! Read the vtk-hdf file type from an hdf5 file
template<typename C>
std::string get_file_type(const HDF5::File<C>& file) {
    if (!file.exists("/VTKHDF"))
        throw IOError("Given file is not a VTK-HDF file");
    if (!file.has_attribute_at("/VTKHDF/Type"))
        throw IOError("VTKHDF-Type attribute missing");
    return file.template read_attribute_to<std::string>("/VTKHDF/Type");
}

//! Check that the version stated in the file is supported
//! \note Only the major version is checked, since the specification guarantees that files can be
//!       read correctly by implementations for a different minor version of the same major.
template<typename C>
void check_version_compatibility(const HDF5::File<C>& file, const std::array<std::size_t, 2>& supported) {
    if (file.has_attribute_at("/VTKHDF/Version"))
        file.visit_attribute("/VTKHDF/Version", [&] (auto&& field) {
            const auto version = field.template export_to<std::vector<std::size_t>>();
            if (version.size() > 0 && version.at(0) > supported[0])
                throw ValueError(
                    "File version is higher than supported by the reader (" + as_string(supported, ".") + ")"
                );
        });
}

/*!
 * \brief Open the given file with the default (non-MPI) driver on the root rank and pass it to `write`.
 * \details Must be called collectively once all ranks have closed the file. If `write` throws, the
 *          exception is rethrown on the root rank, and an IOError is thrown on all other ranks.
 */
template<Concepts::Communicator C, std::invocable<HDF5::File<>&> Write>
void write_on_root(const std::string& filename, const C& comm, int root, Write&& write) {
    Parallel::barrier(comm);
    std::exception_ptr error;
    if (Parallel::rank(comm) == root) {
        try {
            HDF5::File file{filename, HDF5::File<>::append};
            write(file);
        } catch (...) {
            error = std::current_exception();
        }
    }
    // the broadcast also keeps the other ranks from reopening the file before the root is done
    if (Parallel::broadcast(comm, static_cast<int>(error != nullptr), root)) {
        if (error)
            std::rethrow_exception(error);
        throw IOError("Writing to '" + filename + "' failed on rank " + std::to_string(root));
    }
}

#endif  // GRIDFORMAT_HAVE_HIGH_FIVE

}  // namespace VTKHDF
}  // namespace GridFormat

#endif  // GRIDFORMAT_VTK_HDF_COMMON_HPP_
